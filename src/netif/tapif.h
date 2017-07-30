/**
 * based on lwip-contrib
 */
#ifndef LWIP_TAPIF_H
#define LWIP_TAPIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/netif.h"

err_t tapif_init(struct netif *netif);

#if NO_SYS
void tapif_input(struct netif *netif);
#endif /* NO_SYS */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_TAPIF_H */
