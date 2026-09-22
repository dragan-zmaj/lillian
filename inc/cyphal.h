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


#define CYPHAL_NODE_ID 37U
#define CYPHAL_HEAP_SIZE (8u * 1024u)  // Single pool for all allocations
#define CANARD_IFACE_COUNT 1U
#define CYPHAL_TX_QUEUE_CAPACITY    32U     // Max frames in TX queue
#define CYPHAL_MTU_BYTES            CANARD_MTU_CAN_CLASSIC //data length at cyphal to driver level needs to extended if FDCAN is to be used
#define CYPHAL_TRANSFER_ID_TIMEOUT  CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC  // 2 seconds


void cyphalInit(void);

void HeartbeatPublisher(void);

void cyphalPublish(void);

void cyphalRxInit(void);

void processReceivedTransfer(const struct CanardRxTransfer* transfer);



