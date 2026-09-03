// src/canard_platform.c
#include "canard.h"
#include "main.h"
#include "cyphal.h"
#include "o1heap.h"
#include <stdalign.h>
#include <stdint.h>

// Include generated DSDL Cyphal messages
#include "motorRpmSetpoint_1_0.h"
#include "motorStart_1_0.h"





//static alignas(O1HEAP_ALIGNMENT) uint8_t g_o1heapPool[O1HEAP_POOL_SIZE_BYTES];
//static alignas(O1HEAP_ALIGNMENT) uint8_t heap_arena[HEAP_ARENA_SIZE_BYTES];
uint8_t heap_arena[HEAP_ARENA_SIZE_BYTES] __attribute__ ((aligned (O1HEAP_ALIGNMENT)));

// Static storage for heap and ring buffers
//static O1HeapInstance*  g_o1heapInstance = NULL;
static O1HeapInstance* o1heap = NULL;

extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_TxHeaderTypeDef TxHeader; 


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
    return (CanardMicrosecond)HAL_GetTick() * 1000ULL;
}

void cyphalInit(void)
{
    canard = canardInit(memory);
    canard.node_id = CYPHAL_NODE_ID;
    canardTxQueue = canardTxInit(CYPHAL_TX_QUEUE_CAPACITY,CYPHAL_MTU_BYTES,memoryTx);
}


bool cyphalTx(const uint16_t      subject_id,
                       const uint8_t* const payload,
                       const size_t         size,
                       const struct CanardTransferMetadata  priority)
{
    /*
    canardTxPush() does serialization of message into frames and inserts them into tx queue with priority
    canardTxPeek() takes those frames from queue and transmit them
    canardTxPop() removes frame from queue after tx is done or failed
    _____________
    */


}