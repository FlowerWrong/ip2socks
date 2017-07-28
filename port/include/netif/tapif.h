/**
 * based on lwip-contrib
 */
#ifndef LWIP_TAPIF_H
#define LWIP_TAPIF_H

#include "lwip/netif.h"

err_t tapif_init(struct netif *netif);

#if NO_SYS
void tapif_input(struct netif *netif);
#endif /* NO_SYS */

#endif /* LWIP_TAPIF_H */
