// src/canard_platform.c
#include "canRingBuffer.h"
#include "canard.h"
#include "main.h"
#include "cyphal.h"
#include "mc_api.h"
#include "node/Health_1_0.h"
#include "nunavut/support/serialization.h"
#include "o1heap.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Include generated DSDL Cyphal messages
#include "motorControl_1_1.h"
#include "testSampleRate_1_0.h"
#include "node/Heartbeat_1_0.h"
#include "node/GetInfo_1_0.h"

// Include ha
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_tim.h"




//static alignas(O1HEAP_ALIGNMENT) uint8_t g_o1heapPool[O1HEAP_POOL_SIZE_BYTES];
//static alignas(O1HEAP_ALIGNMENT) uint8_t heap_arena[HEAP_ARENA_SIZE_BYTES];
//uint8_t heap_arena[CYPHAL_HEAP_SIZE] __attribute__ ((aligned (CYPHAL_HEAP_SIZE)));
uint8_t base[CYPHAL_HEAP_SIZE] __attribute__ ((aligned (O1HEAP_ALIGNMENT)));

// Static storage for heap and ring buffers
//static O1HeapInstance*  g_o1heapInstance = NULL;
static O1HeapInstance* o1heap = NULL;

extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_TxHeaderTypeDef TxHeader; 
extern canRingBuffer g_canRxRingBuffer;
extern TIM_HandleTypeDef htim6;
extern volatile uint32_t tim6_overflow_count;
//_____________________________________
// Cyphal Rx Messages
struct CanardRxSubscription getInfoSubscription;
uavcan_node_GetInfo_Request_1_0     getInfoRequest;
uavcan_node_GetInfo_Response_1_0  getInfoResponse;
static void setNodeName(const char* name);

struct CanardRxSubscription motorControlSubscription;
cyphalMessages_motorControl_Request_1_1 motorControlRequest = {.targetRpm = 30000};
cyphalMessages_motorControl_Response_1_1 motorControlResponse;

struct CanardRxSubscription testSampleRateSubscription;
cyphalMessages_testSampleRate_1_0   testSampleRate = {.sampleRate = 50000};


//------------------------------------------------------------------------------
// Memory management - o1heap wrappers
static void* memAlloc(void* const memory, const size_t size)
{
    (void)memory;
    return o1heapAllocate(o1heap, size);
}

static void memFree(void* const memory, const size_t size, void* const ptr)
{
    (void)memory;
    (void)size;
    o1heapFree(o1heap, ptr);
}

// Cyphal Init
const struct CanardMemoryResource memory = {NULL, memFree, memAlloc};
const struct CanardMemoryResource memoryTx = {NULL, memFree, memAlloc};
struct CanardInstance canard;
struct CanardTxQueue  canardTxQueue;

static inline CanardMicrosecond cyphalGetTime(void)
{
    uint32_t overflow;
    uint16_t counter;

    do 
    {
        overflow = tim6_overflow_count;
        counter = __HAL_TIM_GET_COUNTER(&htim6);
    }while (overflow != tim6_overflow_count);

    return ((CanardMicrosecond)overflow << 16) | counter;
}

void cyphalInit(void)
{
    o1heap = o1heapInit(base, CYPHAL_HEAP_SIZE);
    canard = canardInit(memory);
    canard.node_id = CYPHAL_NODE_ID;
    canardTxQueue = canardTxInit(CYPHAL_TX_QUEUE_CAPACITY,CYPHAL_MTU_BYTES,memoryTx);


    // init subscribe cyphal messages
    // getInfo - portID = 430 - request/response
    int8_t result;
    result = canardRxSubscribe(&canard,
                            CanardTransferKindRequest,
                            uavcan_node_GetInfo_1_0_FIXED_PORT_ID_,
                            uavcan_node_GetInfo_Request_1_0_EXTENT_BYTES_,
                            CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
                            &getInfoSubscription);
    if (result < 0)
    {
        Error_Handler();
    }
    getInfoResponse.protocol_version.major = CANARD_CYPHAL_SPECIFICATION_VERSION_MAJOR;
    getInfoResponse.protocol_version.minor = CANARD_CYPHAL_SPECIFICATION_VERSION_MINOR;
    getInfoResponse.software_version.minor = SW_MINOR;
    getInfoResponse.software_version.major = SW_MAJOR;
    getInfoResponse.hardware_version.minor = HW_MINOR;
    getInfoResponse.hardware_version.major = HW_MAJOR;
    platformSpecificReadUniqueID(getInfoResponse.unique_id);
    getInfoResponse.software_vcs_revision_id = GIT_HASH;
    getInfoResponse.certificate_of_authenticity.count = 0;
    setNodeName(NODE_NAME);

    //finish init values


    // motorControl - portID = 120 - subscription
    result = canardRxSubscribe(&canard,
                            CanardTransferKindResponse,  //reversed compared to master 
                            cyphalMessages_motorControl_1_1_FIXED_PORT_ID_,
                            cyphalMessages_motorControl_Request_1_1_EXTENT_BYTES_,
                            CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
                            &motorControlSubscription);
    if (result < 0)
    {
        Error_Handler();
    }

        // motorControl - portID = 120 - subscription
    result = canardRxSubscribe(&canard,
                            CanardTransferKindMessage,
                            cyphalMessages_testSampleRate_1_0_FIXED_PORT_ID_,
                            cyphalMessages_testSampleRate_1_0_EXTENT_BYTES_,
                            CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
                            &testSampleRateSubscription);
    if (result < 0)
    {
        Error_Handler();
    }
}



void HeartbeatPublisher(void)
{
    static uint32_t now_1 = 0;
    uint32_t now = HAL_GetTick(); 
    static uint8_t buffer[uavcan_node_Heartbeat_1_0_EXTENT_BYTES_]; 
    size_t bufferSize = uavcan_node_Heartbeat_1_0_EXTENT_BYTES_;
    static CanardTransferID heartbeat_transferID = 0;
    // Cyphal Message - Heartbeat
    uavcan_node_Heartbeat_1_0 heartbeat = {
        .uptime                         = 0U,
        .health.value                   = uavcan_node_Health_1_0_NOMINAL,
        .mode.value                     = uavcan_node_Mode_1_0_OPERATIONAL,
        .vendor_specific_status_code    = 0U
    };

    //executes every second 
    if ((int32_t)(now - now_1) >= 0)
    {
        now_1 = now + 1000;
        heartbeat.uptime = now/1000;  //update uptime of heartbeat to signal its alive

        // serialize message generated by nunavut from dsdl
        if (uavcan_node_Heartbeat_1_0_serialize_(&heartbeat, buffer, &bufferSize) == NUNAVUT_SUCCESS)
        {
            // construct metadata
            const struct CanardTransferMetadata heartbeatMetadata = {
                .transfer_id    = heartbeat_transferID,
                .transfer_kind  = CanardTransferKindMessage,
                .port_id        = uavcan_node_Heartbeat_1_0_FIXED_PORT_ID_, 
                .remote_node_id = CANARD_NODE_ID_UNSET, 
                .priority       = CanardPriorityNominal  
            };
            // Build the payload from the serialized buffer
            struct CanardPayload payload = {
                .size = bufferSize,
                .data = buffer,
            };
            CanardMicrosecond now_usec = cyphalGetTime();
            int32_t result =   canardTxPush(&canardTxQueue,
                                            &canard,
                                            0,
                                            &heartbeatMetadata,
                                            payload,
                                            now_usec,
                                            NULL);
            if (result >= 0) heartbeat_transferID++;
        }
    }
}

void testMotorControlPublisher(void)
{
    static CanardMicrosecond now_1 = 0;
    CanardMicrosecond now = cyphalGetTime();
    static uint8_t buffer[cyphalMessages_motorControl_Request_1_1_EXTENT_BYTES_]; 
    size_t bufferSize = cyphalMessages_motorControl_Request_1_1_EXTENT_BYTES_;
    static CanardTransferID transferID = 0;           

    if ((int32_t)(now - now_1) >= 0) 
    {
        now_1 = now + (CanardMicrosecond)testSampleRate.sampleRate;

        motorControlRequest.startMotor = testSampleRate.startTest;
       
        if (cyphalMessages_motorControl_Request_1_1_serialize_(&motorControlRequest, buffer, &bufferSize) == NUNAVUT_SUCCESS)
        {
            // construct metadata
            const struct CanardTransferMetadata motorControlMetadata = {
                .transfer_id    = transferID,
                .transfer_kind  = CanardTransferKindRequest,
                .port_id        = cyphalMessages_motorControl_1_1_FIXED_PORT_ID_,
                .remote_node_id = DUT_NODE_ID,  
                .priority       = CanardPriorityHigh  
            };  
            // Build the payload from the serialized buffer
            struct CanardPayload payload = {
                .size = bufferSize,
                .data = buffer,
            }; 
            int32_t result =   canardTxPush(&canardTxQueue,
                                            &canard,
                                            0,
                                            &motorControlMetadata,
                                            payload,
                                            now,
                                            NULL);
            if (result >= 0) transferID++;                   
        }


    }


}

void cyphalPublish(void)
{
    struct CanardTxQueueItem* item;
    while ((item = canardTxPeek(&canardTxQueue)) != NULL)
    {
        TxHeader.Identifier = item->frame.extended_can_id;
        TxHeader.DataLength = (uint32_t)item->frame.payload.size; // works only with classic CAN. FDCAN needs switch function.

        if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, item->frame.payload.data) != HAL_OK)
        {
            break; 
        }
        // Pop and Free could be moved to CAN Tx ISR
        canardTxPop(&canardTxQueue, item);
        canardTxFree(&canardTxQueue, &canard, item);
    }
}


//Process received CAN frame
void cyphalProcess(void)
{
    canRxFrame frameRx;
    struct CanardFrame frameRxCanard;

    while (canRingBufferPop(&g_canRxRingBuffer, &frameRx) == true)
    {
        struct CanardRxTransfer transfer;
        frameRxCanard.extended_can_id = frameRx.identifier;
        frameRxCanard.payload.size = frameRx.dlc;
        frameRxCanard.payload.data = frameRx.data;
        
        int8_t result; 
        result = canardRxAccept(&canard, 
                                cyphalGetTime(), 
                                &frameRxCanard, 
                                0, 
                                &transfer,
                                NULL);
        
        if (result == 1)
        {
            processReceivedTransfer(&transfer);  // A transfer has been received, process it.
            //canard.memory_free(&canard, transfer.payload); 
            canard.memory.deallocate(canard.memory.user_reference, transfer.payload.allocated_size, transfer.payload.data);
        }   
        else if (result < 0)     
        {
            Error_Handler();
        }  
        else if (result == 0)
        {
            // frame rejected or multi frame ongoing
        }                  
    }
}

void processReceivedTransfer(const struct CanardRxTransfer* transfer)
{
    size_t size = transfer->payload.size;
    switch(transfer->metadata.port_id)
    {
        case cyphalMessages_testSampleRate_1_0_FIXED_PORT_ID_:
        {
            if(cyphalMessages_testSampleRate_1_0_deserialize_(&testSampleRate, transfer->payload.data, &size) == NUNAVUT_SUCCESS)
            {

            }
        }        
        case cyphalMessages_motorControl_1_1_FIXED_PORT_ID_:
        {   
            if (transfer->metadata.transfer_kind != CanardTransferKindResponse) break;
            if (cyphalMessages_motorControl_Response_1_1_deserialize_(&motorControlResponse, transfer->payload.data, &size) >= 0)
            {
                  
            }                 
            break;
        }
        case uavcan_node_GetInfo_1_0_FIXED_PORT_ID_:
        {
            uint8_t buffer[uavcan_node_GetInfo_Response_1_0_EXTENT_BYTES_];
            size_t bufferSize = uavcan_node_GetInfo_Response_1_0_EXTENT_BYTES_;
            if (transfer->metadata.transfer_kind != CanardTransferKindRequest) break;
            if (uavcan_node_GetInfo_Request_1_0_deserialize_(&getInfoRequest, transfer->payload.data, &size) >= 0)
            {
                if (uavcan_node_GetInfo_Response_1_0_serialize_(&getInfoResponse, buffer, &bufferSize) == NUNAVUT_SUCCESS)
                {
                    const struct CanardTransferMetadata getInfoResponseMetadata = {
                        .priority = CanardPriorityNominal,
                        .transfer_kind = CanardTransferKindResponse,
                        .port_id = uavcan_node_GetInfo_1_0_FIXED_PORT_ID_,
                        .remote_node_id = transfer->metadata.remote_node_id,
                        .transfer_id = transfer->metadata.transfer_id
                    };         
                    struct CanardPayload payload = {
                        .size = bufferSize,
                        .data = buffer
                    };
                    CanardMicrosecond now_usec = cyphalGetTime();
                    int32_t result = canardTxPush(&canardTxQueue,
                                                  &canard,
                                                  0,
                                                  &getInfoResponseMetadata,
                                                  payload,
                                                  now_usec,
                                                  NULL);
                    if (result < 0) Error_Handler();

                }

            }
            break;
        }
        default:
            break;
    }
}


void platformSpecificReadUniqueID(uint8_t *out_uid) {
    const uint32_t UNIQUE_ID_16_BYTES[4] = {
        HAL_GetUIDw0(),
        HAL_GetUIDw1(),
        HAL_GetUIDw2(),
        0
    };
    memcpy(out_uid, UNIQUE_ID_16_BYTES, 16);
}

static void setNodeName(const char* name)
{
    /* Nunavut layout: count is uint8_t, elements is uint8_t[50] */
    uavcan_node_GetInfo_Response_1_0* r = &getInfoResponse;

    size_t len = 0;
    while (name[len] != '\0' && len < 50) {
        r->name.elements[len] = (uint8_t)name[len];
        ++len;
    }
    r->name.count = (uint8_t)len;
}
/*

    if(transfer->metadata.port_id == cyphalMessages_motorControl_1_0_FIXED_PORT_ID_)
    {
        size_t size = transfer->payload.size;
        if (cyphalMessages_motorControl_1_0_deserialize_(&motorControl, transfer->payload.data, &size) >= 0)
        {
            if (motorControl.startMotor == 1)
                MC_StartMotor1();
            else MC_StopMotor1();
        }
    }

*/