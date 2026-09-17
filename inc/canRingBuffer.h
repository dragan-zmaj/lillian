#ifndef CAN_RING_BUFFER_H
#define CAN_RING_BUFFER_H

//#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
//#include <stdatomic.h>

#include "canard.h"

#define CAN_RING_BUFFER_SIZE    32U //must be power-of-2 in order for code to execute in real-time, moduo is implemented as bitwise operation when it is power-of-2
#define CAN_RING_BUFFER_MASK    (CAN_RING_BUFFER_SIZE - 1U)

// @brief SingleProduceSingleConsume Ring Buffer structure
typedef struct {
    struct CanardFrame  buffer[CAN_RING_BUFFER_SIZE];
    _Atomic uint32_t    head;   //Written only by producer (ISR)
    _Atomic uint32_t    tail;   //Written only by consumer (Main/Cyphal)
} canRingBuffer;

// ____________________
// Function Declaration

// @brief Initialize ring buffer 
void canRingBufferInit(canRingBuffer *rb);

// @brief Insert new frame into ring buffer (executed by ISR)
// @return true if frame is pushed, false is buffer is full
bool canRingBufferPush(canRingBuffer *rb, const struct CanardFrame *frm);

// @brief Take frame from ring buffer (executed by cyphal protocol running inside main or task)
// @return true if frame is popped, false is buffer is empty
bool canRingBufferPop(canRingBuffer *rb, struct CanardFrame *frm);

#endif //CAN_RING_BUFFER_H               