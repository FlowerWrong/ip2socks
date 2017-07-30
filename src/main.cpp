#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "ev.h"
#include "yaml.h"

#include "lwip/init.h"
#include "lwip/mem.h"
#include "lwip/timeouts.h"
#include "lwip/ip.h"
#include "lwip/ip4_frag.h"

#include "struct.h"

#if defined(LWIP_UNIX_LINUX)

#include "netif/tapif.h"

#endif

#include "netif/tunif.h"

#include "netif/etharp.h"

#include "udp_raw.h"
#include "tcp_raw.h"

/* (manual) host IP configuration */
static ip4_addr_t ipaddr, netmask, gw;

static char *config_file;
static char *shell_file;

/* nonstatic debug cmd option, exported in lwipopts.h */
unsigned char debug_flags;

static struct option longopts[] = {
  /* turn on debugging output (if build with LWIP_DEBUG) */
  {"debug",  no_argument,       NULL, 'd'},
  /* help */
  {"help",   no_argument,       NULL, 'h'},
  /* config file */
  {"config", required_argument, NULL, 'c'},
  /* shell script */
  {"shell",  required_argument, NULL, 's'},
  /* new command line options go here! */
  {NULL, 0,                     NULL, 0}
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

static void tuntap_read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

struct netif netif;

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
      case 'c':
        config_file = optarg;
            break;
      case 's':
        shell_file = optarg;
            break;
      default:
        usage();
            break;
    }
  }
  argc -= optind;
  argv += optind;

  if (config_file == NULL) {
    printf("Please provide config file\n");
    exit(0);
  }

  printf("config file %s shell file %s\n", config_file, shell_file);


  /**
   * yaml config parser start
   */
  FILE *fh = fopen(config_file, "r");
  yaml_parser_t parser;
  yaml_token_t token;   /* new variable */

  /* Initialize parser */
  if (!yaml_parser_initialize(&parser))
    fputs("Failed to initialize parser!\n", stderr);
  if (fh == NULL)
    fputs("Failed to open file!\n", stderr);

  /* Set input file */
  yaml_parser_set_input_file(&parser, fh);

  /**
   * state = 0 = expect key
   * state = 1 = expect value
   */
  int state = 0;
  char **datap;
  char *tk;

  /* BEGIN new code */
  do {
    yaml_parser_scan(&parser, &token);
    switch (token.type) {
      /* Stream start/end */
      case YAML_STREAM_START_TOKEN:
        puts("STREAM START");
            break;
      case YAML_STREAM_END_TOKEN:
        puts("STREAM END");
            break;
            /* Token types (read before actual token) */
      case YAML_KEY_TOKEN:
        printf("(Key token)   ");
            state = 0;
            break;
      case YAML_VALUE_TOKEN:
        printf("(Value token) ");
            state = 1;
            break;
            /* Block delimeters */
      case YAML_BLOCK_SEQUENCE_START_TOKEN:
        puts("<b>Start Block (Sequence)</b>");
            break;
      case YAML_BLOCK_ENTRY_TOKEN:
        puts("<b>Start Block (Entry)</b>");
            break;
      case YAML_BLOCK_END_TOKEN:
        puts("<b>End block</b>");
            break;
            /* Data */
      case YAML_BLOCK_MAPPING_START_TOKEN:
        puts("[Block mapping]");
            break;
      case YAML_SCALAR_TOKEN:
        printf("scalar %s \n", token.data.scalar.value);
            tk = (char *) token.data.scalar.value;
            if (state == 0) {
              if (strcmp(tk, "ip_mode") == 0) {
                datap = &conf->ip_mode;
              } else if (strcmp(tk, "socks_server") == 0) {
                datap = &conf->socks_server;
              } else if (strcmp(tk, "socks_port") == 0) {
                datap = &conf->socks_port;
              } else if (strcmp(tk, "remote_dns_server") == 0) {
                datap = &conf->remote_dns_server;
              } else if (strcmp(tk, "gw") == 0) {
                datap = &conf->gw;
              } else if (strcmp(tk, "addr") == 0) {
                datap = &conf->addr;
              } else if (strcmp(tk, "netmask") == 0) {
                datap = &conf->netmask;
              } else {
                printf("Unrecognised key: %s\n", tk);
              }
            } else {
              *datap = strdup(tk);
            }
            break;
            /* Others */
      default:
        printf("Got token of type %d\n", token.type);
    }
    if (token.type != YAML_STREAM_END_TOKEN)
      yaml_token_delete(&token);
  } while (token.type != YAML_STREAM_END_TOKEN);
  yaml_token_delete(&token);
  /* END new code */

  /* Cleanup */
  yaml_parser_delete(&parser);
  fclose(fh);

  printf("%s %s\n", conf->remote_dns_server, conf->ip_mode);
  /**
   * yaml config parser end
   */

  /**
   * if config, overside default value
   */
  if (conf->gw != NULL) {
    ip4addr_aton(conf->gw, &gw);
  }
  if (conf->addr != NULL) {
    ip4addr_aton(conf->addr, &ipaddr);
  }
  if (conf->netmask != NULL) {
    ip4addr_aton(conf->netmask, &netmask);
  }

  if (conf->ip_mode == NULL) {
    memcpy(conf->ip_mode, "tun", 3);
  }


  strncpy(ip_str, ip4addr_ntoa(&ipaddr), sizeof(ip_str));
  strncpy(nm_str, ip4addr_ntoa(&netmask), sizeof(nm_str));
  strncpy(gw_str, ip4addr_ntoa(&gw), sizeof(gw_str));
  printf("Host at %s mask %s gateway %s\n", ip_str, nm_str, gw_str);


  /* lwip/src/core/init.c */
  lwip_init();

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
  if (!strcmp(conf->ip_mode, "tun")) {
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

  udp_raw_init();
  tcp_raw_init();

  std::cout << "Ip2socks started!" << std::endl;

  struct ev_io *tuntap_io = (struct ev_io *) mem_malloc(sizeof(struct ev_io));

  if (tuntap_io == NULL) {
    printf("tuntap_io: out of memory for tuntap_io\n");
    return -1;
  }
  struct ev_loop *loop = ev_default_loop(0);

  struct tuntapif *tuntapif;
  tuntapif = (struct tuntapif *) ((&netif)->state);

  ev_io_init(tuntap_io, tuntap_read_cb, tuntapif->fd, EV_READ);
  ev_io_start(loop, tuntap_io);

  // TODO
  sys_check_timeouts();

  return ev_run(loop, 0);
}

static void tuntap_read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  if (strcmp(conf->ip_mode, "tun") == 0) {
    tunif_input(&netif);
  } else {
#if defined(LWIP_UNIX_LINUX)
    tapif_input(&netif);
#endif
  }
  return;
}
