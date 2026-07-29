#ifndef CAN_RING_BUFFER_H
#define CAN_RING_BUFFER_H

//#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
//#include <stdatomic.h>

#define CAN_RING_BUFFER_SIZE    32U //must be power-of-2 in order for code to execute in real-time, moduo is implemented as bitwise operation when it is power-of-2
#define CAN_RING_BUFFER_MASK    (CAN_RING_BUFFER_SIZE - 1U)

// @brief Compact CAN frame representation for internal ring buffering                
typedef struct {
    uint32_t            identifier;            // 29-bit Extended CAN ID
    uint8_t             dlc;                   // Data Length Code (0..8) for classic CAN, (0..64) for FDCAN
    uint8_t             data[8];                // Payload bytes      
} canRxFrame;

// @brief SingleProduceSingleConsume Ring Buffer structure
typedef struct {
    canRxFrame          buffer[CAN_RING_BUFFER_SIZE];
    _Atomic uint32_t    head;   //Written only by producer (ISR)
    _Atomic uint32_t    tail;   //Written only by consumer (Main/Cyphal)
} canRingBuffer;

// ____________________
// Function Declaration

// @brief Initialize ring buffer 
void canRingBufferInit(canRingBuffer *rb);

// @brief Insert new frame into ring buffer (executed by ISR)
// @return true if frame is pushed, false is buffer is full
bool canRingBufferPush(canRingBuffer *rb, const canRxFrame *frm);

// @brief Take frame from ring buffer (executed by cyphal protocol running inside main or task)
// @return true if frame is popped, false is buffer is empty
bool canRingBufferPop(canRingBuffer *rb, canRxFrame *frm);

#endif //CAN_RING_BUFFER_H               