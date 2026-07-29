#include "canRingBuffer.h"
#include "stddef.h"
#include <stdatomic.h>
#include <stdbool.h>

// Compile-time verification of power-of-2 capacity
_Static_assert((CAN_RING_BUFFER_SIZE & CAN_RING_BUFFER_MASK) == 0U,
                "ERROR: CAN_RING_BUFFER_CAPACITY must be power-of-2.");

void canRingBufferInit(canRingBuffer *rb)
{
    if(rb == NULL) return;

    // Reset head and tail to 0
    atomic_store_explicit(&rb->head, 0U, memory_order_relaxed);
    atomic_store_explicit(&rb->tail, 0U, memory_order_relaxed);
}

bool canRingBufferPush(canRingBuffer *rb, const canRxFrame *frm)
{
    if((rb == NULL) || (frm == NULL)) return false;

    //atomic read current pointer position
    const uint32_t currentHead = atomic_load_explicit(&rb->head, memory_order_relaxed); //head is accessed to be modified only by isr, therefore relaxed
    const uint32_t currentTail = atomic_load_explicit(&rb->tail, memory_order_acquire); //tail is read by isr and can be modified by main, therefore acquire

    // check if ring buffer is full
    if ((currentHead - currentTail) >= CAN_RING_BUFFER_SIZE) return false; //buffer overflow, frame dropped

    // calculate circural array index
    // this is bitwise equivalent of mod operation which is valid only when ring buffer has power-of-2 size
    // index = currentHead mod CAN_RING_BUFFER_SIZE
    const uint32_t index = currentHead & CAN_RING_BUFFER_MASK;

    // copy CAN frame to buffer
    rb->buffer[index] = *frm;

    // atomic update of head, memory_order_release guarantees frame is buffered before head is updated
    atomic_store_explicit(&rb->head, currentHead + 1U, memory_order_release);

    return true;
}

bool canRingBufferPop(canRingBuffer *rb, canRxFrame *frm)
{
    if ((rb == NULL) || (frm == NULL)) return false;

    //atomic read current pointer position
    const uint32_t currentTail = atomic_load_explicit(&rb->tail, memory_order_relaxed); //tail can be only modified by main, therefore relaxed
    const uint32_t currentHead = atomic_load_explicit(&rb->head, memory_order_acquire); //head is read by main and can be modified by isr, therefore acquire

    //check if ring buffer is empty
    if (currentHead == currentTail) return false; //nothing to pop out

    // calculate circural array index
    // this is bitwise equivalent of mod operation which is valid only when ring buffer has power-of-2 size
    // index = currentTail mod CAN_RING_BUFFER_SIZE
    const uint32_t index = currentTail & CAN_RING_BUFFER_MASK;

    // copy can frame out of buffer
    *frm = rb->buffer[index];
    
    // atomic updated of head, memory_order_release guarantees frame is extracted from buffer before tail is incremented
    atomic_store_explicit(&rb->tail, currentTail + 1U, memory_order_release);

    return true;
}
