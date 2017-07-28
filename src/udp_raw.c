/**
 * based on lwip-contrib
 */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory.h>
#include <error.h>
#include <time.h>
#include "lwip/opt.h"
#include "lwip/udp.h"

#include "udp_raw.h"

#if LWIP_UDP

static struct udp_pcb *udp_raw_pcb;

typedef struct {
    char *buffer;
    int length;
} response;

int SOCKS_PORT = 1080;
char *SOCKS_ADDR = {"127.0.0.1"};

void tcp_dns_query(void *query, response *buffer, int len) {
  int sock;
  struct sockaddr_in socks_server;
  char tmp[1024];

  memset(&socks_server, 0, sizeof(socks_server));
  socks_server.sin_family = AF_INET;
  socks_server.sin_port = htons(SOCKS_PORT);
  socks_server.sin_addr.s_addr = inet_addr(SOCKS_ADDR);

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    printf("[!] Error creating TCP socket");

  if (connect(sock, (struct sockaddr *) &socks_server, sizeof(socks_server)) < 0)
    printf("[!] Error connecting to proxy");

  // socks handshake
  send(sock, "\x05\x01\x00", 3, 0);
  recv(sock, tmp, 1024, 0);

  srand(time(NULL));

  // select random dns server
  in_addr_t remote_dns = inet_addr("8.8.8.8");
  memcpy(tmp, "\x05\x01\x00\x01", 4);
  memcpy(tmp + 4, &remote_dns, 4);
  memcpy(tmp + 8, "\x00\x35", 2);

  printf("Using DNS server: %s\n", inet_ntoa(*(struct in_addr *) &remote_dns));

  send(sock, tmp, 10, 0);
  recv(sock, tmp, 1024, 0);

  // forward dns query
  send(sock, query, len, 0);
  buffer->length = recv(sock, buffer->buffer, 2048, 0);
}

/**
 * receive callback for a UDP PCB
 * pcb->recv(pcb->recv_arg, pcb, p, ip_current_src_addr(), src_port)
 */
static void
udp_raw_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
             const ip_addr_t *addr, u16_t port) {
  LWIP_UNUSED_ARG(arg);
  if (p != NULL) {
    char localip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(upcb->local_ip), localip_str, INET_ADDRSTRLEN);
    char remoteip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(upcb->remote_ip), remoteip_str, INET_ADDRSTRLEN);

    // flow 119.23.211.95:80 <-> 172.16.0.1:53536
    printf("<======== udp flow %s:%d <-> %s:%d\n", localip_str, upcb->local_port, remoteip_str, upcb->remote_port);
    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, addr, addr_str, INET_ADDRSTRLEN);
    printf("<======== addr:port %s:%d\n", addr_str, port);


    response *buffer = (response *) malloc(sizeof(response));
    buffer->buffer = malloc(2048);
    char *query;

    pbuf_copy_partial(p, buffer->buffer, p->tot_len, 0);

    query = malloc(p->len + 3);
    query[0] = 0;
    query[1] = (char) p->len;
    memcpy(query + 2, buffer->buffer, p->len);

    // forward the packet to the tcp dns server
    tcp_dns_query(query, buffer, p->len + 2);

    if (buffer->length > 0) {
      /* send received packet back to sender */
      struct pbuf *socksp = pbuf_alloc(PBUF_TRANSPORT, (uint16_t) buffer->length - 2, PBUF_RAM);
      memcpy(socksp->payload, buffer->buffer + 2, (size_t) buffer->length - 2);

      udp_sendto(upcb, socksp, addr, port);
      /* free the pbuf */
      pbuf_free(socksp);
    }

    free(buffer->buffer);
    free(buffer);
    free(query);
    pbuf_free(p);
  }
}

void
udp_raw_init(void) {
  /* call udp_new */
  udp_raw_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (udp_raw_pcb != NULL) {
    err_t err;

    /* lwip/src/core/udp.c add udp_pcb to udp_pcbs */
    ip4_addr_t ipaddr;
    IP4_ADDR(&ipaddr, 10, 0, 0, 2);
    err = udp_bind(udp_raw_pcb, &ipaddr, 53);
//    err = udp_bind(udp_raw_pcb, IP_ANY_TYPE, 53);
    if (err == ERR_OK) {
      /**
       * lwip/src/core/udp.c
       * @ingroup udp_raw
       * Set a receive callback for a UDP PCB
       *
       * This callback will be called when receiving a datagram for the pcb.
       *
       * @param pcb the pcb for which to set the recv callback
       * @param recv function pointer of the callback function
       * @param recv_arg additional argument to pass to the callback function
       */
      udp_recv(udp_raw_pcb, udp_raw_recv, NULL);
    } else {
      /* abort? output diagnostic? */
    }
  } else {
    /* abort? output diagnostic? */
  }
}

#endif /* LWIP_UDP */
