// src/canard_platform.c
#include "main.h"
#include "cyphal.h"
#include <stdalign.h>


// Memory pool definitions using O1Heap
#define HEAP_ARENA_SIZE_BYTES (8u * 1024u)  // Single pool for all allocations
#define CANARD_IFACE_COUNT 1U


// ==================== Allocator abstraction ====================

typedef void* (*AllocFunc)(void* ctx, size_t size);
typedef void (*FreeFunc)(void* ctx, void* ptr);

typedef struct
{
    const char* name;
    void*       ctx;
    AllocFunc   alloc;
    FreeFunc    free;
    bool        has_diagnostics;
} Allocator;

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



const struct CanardMemoryResource memory = {NULL, memFree, memAlloc};




