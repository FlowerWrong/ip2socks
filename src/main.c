#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ev.h"
#include "sds.h"

#include "lwip/init.h"
#include "lwip/mem.h"
#include "lwip/timeouts.h"
#include "lwip/ip.h"
#include "lwip/ip4_frag.h"

#if defined(LWIP_UNIX_LINUX)
#include "netif/tapif.h"
#endif

#include "netif/tunif.h"

#include "netif/etharp.h"

#include "udpecho_raw.h"
#include "tcpecho_raw.h"

/* (manual) host IP configuration */
static ip4_addr_t ipaddr, netmask, gw;


/* nonstatic debug cmd option, exported in lwipopts.h */
unsigned char debug_flags;

static struct option longopts[] = {
  /* turn on debugging output (if build with LWIP_DEBUG) */
  {"debug",            no_argument,       NULL, 'd'},
  /* help */
  {"help",             no_argument,       NULL, 'h'},
  /* gateway address */
  {"gateway",          required_argument, NULL, 'g'},
  /* ip address */
  {"ipaddr",           required_argument, NULL, 'i'},
  /* netmask */
  {"netmask",          required_argument, NULL, 'm'},
  /* ping destination */
  {"trap_destination", required_argument, NULL, 't'},
  /* new command line options go here! */
  {NULL, 0,                               NULL, 0}
};
#define NUM_OPTS ((sizeof(longopts) / sizeof(struct option)) - 1)

static void
usage(void) {
  unsigned char i;

  printf("options:\n");
  for (i = 0; i < NUM_OPTS; i++) {
    printf("-%c --%s\n", longopts[i].val, longopts[i].name);
  }
}

#define BUFFER_SIZE 1514

static void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

struct netif netif;

struct tuntapif {
    /* Add whatever per-interface state that is needed here. */
    int fd;
};

/**
 * 1 tun
 * 2 tap
 */
#define TUNTAP 1

int
main(int argc, char **argv) {
  int ch;
  char ip_str[16] = {0}, nm_str[16] = {0}, gw_str[16] = {0};

  /* startup defaults (may be overridden by one or more opts) */
  IP4_ADDR(&gw, 10, 0, 0, 1);
  // ip4_addr_set_any(&gw);
  ip4_addr_set_any(&ipaddr);
  IP4_ADDR(&ipaddr, 10, 0, 0, 2);
  IP4_ADDR(&netmask, 255, 255, 255, 0);

  /* use debug flags defined by debug.h */
  debug_flags = LWIP_DBG_OFF;

  while ((ch = getopt_long(argc, argv, "dhg:i:m:t:", longopts, NULL)) != -1) {
    switch (ch) {
      case 'd':
        debug_flags |= (LWIP_DBG_ON | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_FRESH | LWIP_DBG_HALT);
            break;
      case 'h':
        usage();
            exit(0);
            break;
      case 'g':
        ip4addr_aton(optarg, &gw);
            break;
      case 'i':
        ip4addr_aton(optarg, &ipaddr);
            break;
      case 'm':
        ip4addr_aton(optarg, &netmask);
            break;
      case 't':
        break;
      default:
        usage();
            break;
    }
  }
  argc -= optind;
  argv += optind;

  strncpy(ip_str, ip4addr_ntoa(&ipaddr), sizeof(ip_str));
  strncpy(nm_str, ip4addr_ntoa(&netmask), sizeof(nm_str));
  strncpy(gw_str, ip4addr_ntoa(&gw), sizeof(gw_str));
  printf("Host at %s mask %s gateway %s\n", ip_str, nm_str, gw_str);


  /* lwip/src/core/init.c */
  lwip_init();

  printf("TCP/IP initialized.\n");

  sds sds_str = sdsnew("sds worked");
  printf("%s\n", sds_str);
  sdsfree(sds_str);

  /*
    netif_add lwip/core/netif.c
    tapif_init port/netif/tapif.c -> netif->output = etharp_output and netif->linkoutput = low_level_output
    ethernet_input lwip/src/netif/ethernet.c
    ethernet_output lwip/src/netif/ethernet.c Send an ethernet packet on the network using netif->linkoutput()
    ip4_input lwip/core/ipv4/ip4.c
                                          -> tcp_input(p, inp)
    ethernet_input -> ip4_input(p, netif)
                                          -> upd_input -> pcb->recv(pcb->recv_arg, pcb, p, ip_current_src_addr(), src) in udp_pcb *udp_pcbs -> udp_recv_callback -> udp_sendto -> udp_sendto_if(組裝成udp packet) -> ip4_output_if(組裝成ip packet) -> netif->output(netif, p, dest)
  */
  if (TUNTAP == 1) {
    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tunif_init, ip_input); // IPV4 IPV6 TODO
  } else {
#if defined(LWIP_UNIX_LINUX)
    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tapif_init, ethernet_input);
#endif
  }
  netif_set_default(&netif);

  netif_set_link_up(&netif);

  /* lwip/core/netif.c */
  netif_set_up(&netif);
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(&netif, 1);
#endif

  udpecho_raw_init();
  tcpecho_raw_init();

  printf("Applications started.\n");

  struct ev_io *tuntap_io = (struct ev_io *) mem_malloc(sizeof(struct ev_io));

  if (tuntap_io == NULL) {
    printf("tuntap_io: out of memory for tuntap_io\n");
    return -1;
  }
  struct ev_loop *loop = ev_default_loop(0);

  struct tuntapif *tuntapif;
  tuntapif = (struct tuntapif *) ((&netif)->state);

  ev_io_init(tuntap_io, read_cb, tuntapif->fd, EV_READ);
  ev_io_start(loop, tuntap_io);

  // TODO
  // sys_check_timeouts();

  return ev_run(loop, 0);
}

static void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  if (TUNTAP == 1) {
    tunif_input(&netif);
  } else {
#if defined(LWIP_UNIX_LINUX)
    tapif_input(&netif);
#endif
  }
  return;
}
