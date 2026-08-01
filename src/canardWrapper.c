// src/canard_platform.c
#include "o1heap.h"
#include "canRingBuffer.h"
#include <stdalign.h>
#include "stm32g4xx_hal.h"

// Memory pool definitions using O1Heap
#define O1HEAP_POOL_SIZE_BYTES        4096U  // Single pool for all allocations
#define CANARD_IFACE_COUNT 1U
#include "canardWrapper.h"

static alignas(O1HEAP_ALIGNMENT) uint8_t g_o1heapPool[O1HEAP_POOL_SIZE_BYTES];

// Static storage for heap and ring buffers
static O1HeapInstance*  g_o1heapInstance = NULL;

extern FDCAN_HandleTypeDef hfdcan1;
extern FDCAN_TxHeaderTypeDef TxHeader; 


//------------------------------------------------------------------------------
// Memory management - o1heap wrappers
static void* memAlloc(canard_mem_t memory, size_t size)
{
    (void)memory;
    return o1heapAllocate(g_o1heapInstance, size);
}

static void memFree(canard_mem_t memory, size_t size, void* ptr)
{
    (void)memory;
    (void)size;
    o1heapFree(g_o1heapInstance, ptr);
}

/* Global memory vtable instance */
static const canard_mem_vtable_t g_mem_vtable = { 
    .free = memFree, 
    .alloc = memAlloc 
};


static canard_t g_canard;
// Time source - returns monotonic timestamp in microseconds
static canard_us_t vtableNow(const canard_t* const self)
{
    (void)self;
    /* Basic 1ms resolution scaled to microseconds. 
       (For higher precision, use DWT->CYCCNT or a hardware timer) */
    return (canard_us_t)HAL_GetTick() * 1000U; 
}

// Wrapper for transmission
static bool vtableTx(canard_t* const      self,
                      void* const          user_context,
                      const canard_us_t    deadline,
                      const uint_least8_t  iface_index,
                      const bool           fd,
                      const uint32_t       extended_can_id,
                      const canard_bytes_t can_data)
{
    (void)self;
    (void)user_context;
    (void)deadline;
    (void)iface_index;

    TxHeader.Identifier = extended_can_id;
    //TxHeader.DataLength = ((uint32_t)can_data.size << 16U);
    return (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, (uint8_t*)can_data.data) == HAL_OK);
}

static const canard_vtable_t g_canard_vtable = {
    .now    = vtableNow,
    .tx     = vtableTx,
    .filter = NULL
};

// --- SUBSCRIPTIONS AND CALLBACKS --- 
static canard_subscription_t g_sub_heartbeat;

static void on_heartbeat_msg(canard_subscription_t* const self,
                             const canard_us_t            timestamp,
                             const canard_prio_t          priority,
                             const uint_least8_t          source_node_id,
                             const uint_least8_t          transfer_id,
                             const canard_payload_t       payload)
{
    (void)self; (void)timestamp; (void)priority;
    (void)source_node_id; (void)transfer_id;

    // Process Heartbeat Payload...

    // Mandatory: Release memory allocated during multi-frame reassembly
    if (payload.origin.data != NULL) {
        o1heapFree(g_o1heapInstance, payload.origin.data);
    }
}

static const canard_subscription_vtable_t g_sub_vtable_heartbeat = {
    .on_message = on_heartbeat_msg
};

/* --- PUBLIC WRAPPER FUNCTIONS --- */

bool canardWrapperInit(void)
{
    g_o1heapInstance = o1heapInit(g_o1heapPool, sizeof(g_o1heapPool));
    if (g_o1heapInstance == NULL) {
        return false;
    }

    const canard_mem_t mem = { .vtable = &g_mem_vtable, .context = NULL };
    const canard_mem_set_t mem_set = {
        .tx_transfer = mem,
        .tx_frame    = mem,
        .rx_session  = mem,
        .rx_payload  = mem,
        .rx_filters  = mem
    };

    if (!canard_new(&g_canard,
                    &g_canard_vtable,
                    mem_set,
                    1U,   // Iface bitmap (interface 0 active)
                    64U,  // TX queue capacity (frames)
                    0U,   // PRNG seed
                    0U))  // Filter count
    {
        return false;
    }

    // Set local node-ID (e.g., node 42)
    (void)canard_set_node_id(&g_canard, CYPHAL_NODE_ID);

    // Subscribe to Heartbeat (Subject 7509)
    if (canard_subscribe_13b(&g_canard,
                             &g_sub_heartbeat,
                             7509U,
                             7U, // Extent
                             CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_us,
                             &g_sub_vtable_heartbeat) != &g_sub_heartbeat)
    {
        return false;
    }

    return true;
}

void canardWrapperProcess(canRingBuffer* const rx_ring)
{
    canRxFrame rx_frame;

    // 1. Flush all frames from SPSC ring buffer into Libcanard ingest engine
    while (canRingBufferPop(rx_ring, &rx_frame))
    {
        const canard_bytes_t bytes = {
            .size = rx_frame.dlc,
            .data = rx_frame.data
        };

        (void)canard_ingest_frame(&g_canard,
                                  vtableNow(&g_canard),
                                  0U, // Interface index
                                  rx_frame.identifier,
                                  bytes);
    }

    // 2. Housekeeping: handle timeouts and transmit pending TX queue
    canard_poll(&g_canard, 1U);
}

bool canardWrapperPublish_13b(const uint16_t      subject_id,
                              const uint8_t* const payload,
                              const size_t         size,
                              const canard_prio_t  priority)
{
    static uint8_t transfer_id = 0;

    const canard_bytes_chain_t chain = {
        .bytes = { .size = size, .data = payload },
        .next  = NULL
    };

    const canard_us_t deadline = vtableNow(&g_canard) + 1000000LL; // 1 second deadline

    bool ok = canard_publish_13b(&g_canard,
                                 deadline,
                                 CANARD_IFACE_BITMAP_ALL,
                                 priority,
                                 subject_id,
                                 transfer_id++,
                                 chain,
                                 NULL);

    return ok;
}



