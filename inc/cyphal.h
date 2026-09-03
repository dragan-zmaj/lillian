#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "canard.h"
#include "canRingBuffer.h"
#include "o1heap.h"


/*
inkluduj header fajl sa generisanim porukama
ali s obzirom da se radi o generisanim porukama koje mogu da se zovu bilo kako,
mi u sustini rucnim inkludovanjem u wrapper code cyphal.c pravimo beskorisan
hard coded file koji prirodno zahteva repetivne akcije u toku razvoja gde se pisu
beskrajni kodovi. potreban je bilt time include generisan fajl :D zvuci nemoguce al to je cilj
*/


#define CYPHAL_NODE_ID 42U
#define HEAP_ARENA_SIZE_BYTES (8u * 1024u)  // Single pool for all allocations
#define CANARD_IFACE_COUNT 1U
#define CYPHAL_TX_QUEUE_CAPACITY    32U     // Max frames in TX queue
#define CYPHAL_MTU_BYTES            CANARD_MTU_CAN_CLASSIC
#define CYPHAL_TRANSFER_ID_TIMEOUT  CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC  // 2 seconds


void cyphalInit(void);


void cyphalRx(canRingBuffer* const rx_ring);


bool cyphalTx(const uint16_t      subject_id,
                       const uint8_t* const payload,
                       const size_t         size,
                       const struct CanardTransferMetadata  priority);


