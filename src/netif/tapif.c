/**
 * based on lwip-contrib
 */
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "lwip/opt.h"
#include "lwip/ip.h"
#include "lwip/snmp.h"
#include "netif/etharp.h"

#if defined(LWIP_DEBUG) && defined(LWIP_TCPDUMP)
#include "netif/tcpdump.h"
#endif /* LWIP_DEBUG && LWIP_TCPDUMP */

#include "netif/tapif.h"
#include "netif/socket_util.h"

#define IFCONFIG_BIN "/sbin/ifconfig "

#if defined(LWIP_UNIX_LINUX)
#include <linux/if.h>
#include <linux/if_tun.h>
/*
 * Creating a tap interface requires special privileges. If the interfaces
 * is created in advance with `tunctl -u <user>` it can be opened as a regular
 * user. The network must already be configured. If DEVTAP_IF is defined it
 * will be opened instead of creating a new tap device.
 */
#ifndef DEVTAP
#define DEVTAP "/dev/net/tun"
#endif

#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "%s inet %d.%d.%d.%d " NETMASK_ARGS
#elif defined(LWIP_UNIX_OPENBSD)
#define DEVTAP "/dev/tun0"
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "%s inet %d.%d.%d.%d " NETMASK_ARGS " link0"
#else /* others */
#define DEVTAP "/dev/tap0"
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "%s inet %d.%d.%d.%d " NETMASK_ARGS
#endif

/* Define those to better describe your network interface. */
#define IFNAME0 't'
#define IFNAME1 'p'

#ifndef TAPIF_DEBUG
#define TAPIF_DEBUG LWIP_DBG_OFF
#endif

struct tapif {
    /* Add whatever per-interface state that is needed here. */
    int fd;
};

/* Forward declarations. */
void tapif_input(struct netif *netif);

int tap_create(char *dev) {
  int fd = -1;

  if ((fd = open(DEVTAP, O_RDWR)) < 0) {
    printf("open %s failed\n", DEVTAP);
    return fd;
  }

#ifdef LWIP_UNIX_LINUX
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

  if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
    printf("failed to open tap device\n");
    close(fd);
    return -1;
  }
  strcpy(dev, ifr.ifr_name);

  printf("Open tun device: %s for reading...\n", ifr.ifr_name);
#endif /* LWIP_UNIX_LINUX */
  return fd;
}

/*-----------------------------------------------------------------------------------*/
static void
low_level_init(struct netif *netif) {
  struct tapif *tapif;
#if LWIP_IPV4
  int ret;
  char buf[1024];
#endif /* LWIP_IPV4 */
  tapif = (struct tapif *) netif->state;

#if defined(LWIP_UNIX_LINUX)
  char tap_name[IFNAMSIZ];
#endif
  tap_name[0] = '\0';

  /* Obtain MAC address from network interface. */

  /* (We just fake an address...) */
  netif->hwaddr[0] = 0x02;
  netif->hwaddr[1] = 0x12;
  netif->hwaddr[2] = 0x34;
  netif->hwaddr[3] = 0x56;
  netif->hwaddr[4] = 0x78;
  netif->hwaddr[5] = 0xab;
  netif->hwaddr_len = 6;

  /* device capabilities */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

  tapif->fd = tap_create(tap_name);
  LWIP_DEBUGF(TAPIF_DEBUG, ("tapif_init: fd %d\n", tapif->fd));
  if (tapif->fd == -1) {
#ifdef LWIP_UNIX_LINUX
    perror("tapif_init: try running \"modprobe tun\" or rebuilding your kernel with CONFIG_TUN; cannot open "DEVTAP);
#else /* LWIP_UNIX_LINUX */
    perror("tapif_init: cannot open "DEVTAP);
#endif /* LWIP_UNIX_LINUX */
    exit(1);
  }

  setnonblocking(tapif->fd);

  netif_set_link_up(netif);

#if LWIP_IPV4
  snprintf(buf, 1024, IFCONFIG_BIN
    IFCONFIG_ARGS,
           tap_name,
           ip4_addr1(netif_ip4_gw(netif)),
           ip4_addr2(netif_ip4_gw(netif)),
           ip4_addr3(netif_ip4_gw(netif)),
           ip4_addr4(netif_ip4_gw(netif)),
           ip4_addr1(netif_ip4_netmask(netif)),
           ip4_addr2(netif_ip4_netmask(netif)),
           ip4_addr3(netif_ip4_netmask(netif)),
           ip4_addr4(netif_ip4_netmask(netif))
  );

  LWIP_DEBUGF(TAPIF_DEBUG, ("tapif_init: system(\"%s\");\n", buf));
  ret = system(buf);
  if (ret < 0) {
    perror("ifconfig failed");
    exit(1);
  }
  if (ret != 0) {
    printf("ifconfig returned %d\n", ret);
  }
#else /* LWIP_IPV4 */
  perror("todo: support IPv6 support for non-preconfigured tapif");
  exit(1);
#endif /* LWIP_IPV4 */
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/

static err_t
low_level_output(struct netif *netif, struct pbuf *p) {
  struct tapif *tapif = (struct tapif *) netif->state;
  char buf[1514];
  ssize_t written;

#if 0
  if (((double)rand()/(double)RAND_MAX) < 0.2) {
    printf("drop output\n");
    return ERR_OK;
  }
#endif

  /* initiate transfer(); */
  pbuf_copy_partial(p, buf, p->tot_len, 0);

  /* signal that packet should be sent(); */
  written = write(tapif->fd, buf, p->tot_len);
  if (written == -1) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    perror("tapif: write");
  } else {
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, written);
  }
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *
low_level_input(struct netif *netif) {
  struct pbuf *p;
  u16_t len;
  char buf[1514];
  struct tapif *tapif = (struct tapif *) netif->state;

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  len = read(tapif->fd, buf, sizeof(buf));
  if (len == (u16_t) -1) {
    perror("read returned -1");
    exit(1);
  }

  MIB2_STATS_NETIF_ADD(netif, ifinoctets, len);

#if 0
  if (((double)rand()/(double)RAND_MAX) < 0.2) {
    printf("drop\n");
    return NULL;
  }
#endif

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  if (p != NULL) {
    pbuf_take(p, buf, len);
    /* acknowledge that packet has been read(); */
  } else {
    /* drop packet(); */
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
    LWIP_DEBUGF(NETIF_DEBUG, ("tapif_input: could not allocate pbuf\n"));
  }

  return p;
}

/*-----------------------------------------------------------------------------------*/
/*
 * tapif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
tapif_input(struct netif *netif) {
  struct pbuf *p = low_level_input(netif);

  if (p == NULL) {
#if LINK_STATS
    LINK_STATS_INC(link.recv);
#endif /* LINK_STATS */
    LWIP_DEBUGF(TAPIF_DEBUG, ("tapif_input: low_level_input returned NULL\n"));
    return;
  }

  err_t err = netif->input(p, netif);
  if (err != ERR_OK) {
    printf("============================> tapif_input: netif input error %d\n", err);
    pbuf_free(p);
  }
}
/*-----------------------------------------------------------------------------------*/
/*
 * tapif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
tapif_init(struct netif *netif) {
  struct tapif *tapif = (struct tapif *) mem_malloc(sizeof(struct tapif));

  if (tapif == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("tapif_init: out of memory for tapif\n"));
    return ERR_MEM;
  }
  netif->state = tapif;
  MIB2_INIT_NETIF(netif, snmp_ifType_other, 100000000);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = low_level_output;
  netif->mtu = 1500;

  low_level_init(netif);

  return ERR_OK;
}
