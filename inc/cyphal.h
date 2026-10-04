#include <stdbool.h>
#include <stddef.h>
#include "canard.h"
#include "motorControl_1_1.h"



/*
inkluduj header fajl sa generisanim porukama
ali s obzirom da se radi o generisanim porukama koje mogu da se zovu bilo kako,
mi u sustini rucnim inkludovanjem u wrapper code cyphal.c pravimo beskorisan
hard coded file koji prirodno zahteva repetivne akcije u toku razvoja gde se pisu
beskrajni kodovi. potreban je bilt time include generisan fajl :D zvuci nemoguce al to je cilj
*/

#define CYPHAL_NODE_ID 37U
#define NODE_NAME "lillian"
#define GIT_HASH 0x5df71   //0x5df71f5a57ddbe96c543f62533a5e1a5b1e8626b

#define SW_MAJOR    1U
#define SW_MINOR    0U
#define HW_MAJOR    3U  //MB1419C
#define HW_MINOR    0U

#define CYPHAL_HEAP_SIZE (8u * 1024u)  // Single pool for all allocations
#define CANARD_IFACE_COUNT 1U
#define CYPHAL_TX_QUEUE_CAPACITY    32U     // Max frames in TX queue
#define CYPHAL_MTU_BYTES            CANARD_MTU_CAN_CLASSIC //data length at cyphal to driver level needs to extended if FDCAN is to be used
#define CYPHAL_TRANSFER_ID_TIMEOUT  CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC  // 2 seconds

extern cyphalMessages_motorControl_Request_1_1 motorControlRequest;

void cyphalInit(void);

void HeartbeatPublisher(void);

void cyphalPublish(void);

void cyphalProcess(void);

void processReceivedTransfer(const struct CanardRxTransfer* transfer);







void platformSpecificReadUniqueID(uint8_t *out_uid);



#ifndef GIT_HASH
    #define GIT_HASH 0xBADC0FFEE000
#endif
#if GIT_HASH == 0xBADC0FFEE000
    #warning "GIT_HASH has not been provided!"
#endif