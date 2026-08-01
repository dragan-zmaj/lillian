#ifndef CANARD_WRAPPER_H
#define CANARD_WRAPPER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "canard.h"
#include "canRingBuffer.h" // Your SPSC ring buffer header

#ifdef __cplusplus
extern "C" {
#endif

#define CYPHAL_NODE_ID 42U

/**
 * @brief Initialize O1Heap memory, Libcanard instance, and default subscriptions.
 * @return true on success, false on initialization error.
 */
bool canardWrapperInit(void);

/**
 * @brief Drain the SPSC Ring Buffer into Libcanard and execute poll.
 *        Call this repeatedly in the main loop.
 */
void canardWrapperProcess(canRingBuffer* const rx_ring);

/**
 * @brief Publish a 13-bit subject-ID Cyphal message.
 */
bool canardWrapperPublish_13b(const uint16_t      subject_id,
                              const uint8_t* const payload,
                              const size_t         size,
                              const canard_prio_t  priority);

#ifdef __cplusplus
}
#endif

#endif // CANARD_WRAPPER_H