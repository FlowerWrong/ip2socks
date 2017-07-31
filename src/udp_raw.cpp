/**
 * based on lwip-contrib
 */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory.h>
#include <time.h>
#include "lwip/opt.h"
#include "lwip/udp.h"
#include "lwip/ip.h"

#include "udp_raw.h"
#include "struct.h"
#include "socks5.h"

#if LWIP_UDP

static struct udp_pcb *udp_raw_pcb;

typedef struct {
    char *buffer;
    ssize_t length;
} response;

int tcp_dns_query(void *query, response *buffer, int len) {
  int sock = socks5_connect(conf->socks_server, conf->socks_port);
  if (sock < 1) {
    printf("socks5 connect failed\n");
    return -1;
  }

  int ret = socks5_auth(sock, conf->remote_dns_server, "53", 0x01, 1);
  if (ret < 0) {
    printf("socks5 auth failed\n");
    return -1;
  }

  // forward dns query
  send(sock, query, len, 0);
  buffer->length = recv(sock, buffer->buffer, 4096, 0);
  return sock;
}

int udp_relay(void *query, response *buffer, int len) {
  int socks_fd = socks5_connect(conf->socks_server, conf->socks_port);
  if (socks_fd < 1) {
    printf("socks5 connect failed\n");
    return -1;
  }

  char buff[BUFFER_SIZE];
  /**
   * socks 5 method request start
   */
  ((socks5_method_req_t *) buff)->ver = SOCKS5_VERSION;
  ((socks5_method_req_t *) buff)->nmethods = 0x01;
  ((socks5_method_req_t *) buff)->methods[0] = 0x00;

  send(socks_fd, buff, 3, 0);

  // VERSION and METHODS
  if (-1 == recv(socks_fd, buff, 2, 0)) {
    printf("recv VERSION and METHODS error\n");
    return -1;
  };
  if (SOCKS5_VERSION != ((socks5_method_res_t *) buff)->ver || 0x00 != ((socks5_method_res_t *) buff)->method) {
    printf("socks5_method_res_t error\n");
    return -1;
  }
  /**
   * socks 5 method request end
   */

  /**
   * socks 5 request start
   */
  unsigned char socksreq[600];
  int idx;

  idx = 0;
  socksreq[idx++] = 5; /* version */
  socksreq[idx++] = 3; /* udp */
  socksreq[idx++] = 0;
  socksreq[idx++] = 1; /* ATYP: IPv4 = 1 */

  struct sockaddr_in *saddr_in = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));;
  saddr_in->sin_family = AF_INET;
  inet_aton(conf->remote_dns_server, &(saddr_in->sin_addr));
  for (int i = 0; i < 4; i++) {
    socksreq[idx++] = ((unsigned char *) &saddr_in->sin_addr.s_addr)[i];
  }

  int p = atoi(conf->socks_port);
  socksreq[idx++] = (unsigned char) ((p >> 8) & 0xff); /* PORT MSB */
  socksreq[idx++] = (unsigned char) (p & 0xff);        /* PORT LSB */

  send(socks_fd, (char *) socksreq, idx, 0);
  free(saddr_in);

  /**
   * socks 5 request end
   */

  /**
   * socks 5 response
   */
  if (-1 == recv(socks_fd, buff, 10, 0)) {
    printf("recv socks 5 response error\n");
    return -1;
  };

  if (SOCKS5_VERSION != ((socks5_response_t *) buff)->ver) {
    printf("socks 5 response version error\n");
    return -1;
  }

  char *udp_ip_str = inet_ntoa(*(in_addr *) (&buff[4]));
  int udp_port = ntohs(*(short *) (&buff[8]));
  printf("udp %s:%d\n", udp_ip_str, udp_port);

  struct sockaddr_in socks_proxy_addr;

  socks_proxy_addr.sin_family = AF_INET;
  socks_proxy_addr.sin_addr.s_addr = inet_addr(udp_ip_str);
  socks_proxy_addr.sin_port = htons(udp_port);


  // TODO
  unsigned char udp_req[4096];
  int udp_index;

  udp_index = 0;
  udp_req[udp_index++] = 0; /* RSV */
  udp_req[udp_index++] = 0; /* RSV */
  udp_req[udp_index++] = 0; /* FRAG */
  udp_req[udp_index++] = 1; /* ATYP: IPv4 = 1 */

  struct sockaddr_in *udp_saddr_in = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));;
  udp_saddr_in->sin_family = AF_INET;
  inet_aton(conf->remote_dns_server, &(udp_saddr_in->sin_addr));
  for (int i = 0; i < 4; i++) {
    udp_req[udp_index++] = ((unsigned char *) &udp_saddr_in->sin_addr.s_addr)[i];
  }

  p = 53;
  udp_req[udp_index++] = (unsigned char) ((p >> 8) & 0xff); /* PORT MSB */
  udp_req[udp_index++] = (unsigned char) (p & 0xff);        /* PORT LSB */

  memcpy(udp_req + udp_index, query, len);

  int udp_relay_fd = socket(AF_INET, SOCK_DGRAM, 0);

  sockaddr_in localAddr;
  memset(&localAddr, 0, sizeof(localAddr));
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(50000);
  if (bind(udp_relay_fd, (struct sockaddr *) &localAddr, sizeof(localAddr))) {

  }

  int nread = sendto(udp_relay_fd, udp_req, len + udp_index, 0, (struct sockaddr *) (&socks_proxy_addr),
                     sizeof(socks_proxy_addr));
  if (nread < 0) {
    printf("udp query sendto failed\n");
    return -1;
  }

  buffer->length = recvfrom(udp_relay_fd, buffer->buffer, 4096, 0, (struct sockaddr *) (&socks_proxy_addr),
                            reinterpret_cast<socklen_t *>(sizeof(socks_proxy_addr)));
  if (buffer->length < 0) {
    printf("udp data recvfrom failed\n");
    return -1;
  }
  printf("recv buffer->length %d\n", buffer->length);

  return udp_relay_fd;
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
    upcb->so_options |= SO_REUSEADDR;

    response *buffer = (response *) malloc(sizeof(response));
    buffer->buffer = static_cast<char *>(malloc(2048));
    char *query;

    pbuf_copy_partial(p, buffer->buffer, p->tot_len, 0);

    query = static_cast<char *>(malloc(p->len + 3));
    query[0] = 0;
    query[1] = (char) p->len;
    memcpy(query + 2, buffer->buffer, p->len);

    // forward the packet to the tcp dns server
    int socks_fd = tcp_dns_query(query, buffer, p->len + 2);
    // int socks_fd = udp_relay(query, buffer, p->len + 2);

    if (buffer->length > 0) {
      /* send received packet back to sender */
      struct pbuf *socksp = pbuf_alloc(PBUF_TRANSPORT, (uint16_t) buffer->length - 2, PBUF_RAM);
      memcpy(socksp->payload, buffer->buffer + 2, (size_t) buffer->length - 2);

      udp_sendto(upcb, socksp, addr, port);
      /* free the pbuf */
      pbuf_free(socksp);
    }

    // close socks dns socket
    close(socks_fd);

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
    ip4addr_aton(conf->addr, &ipaddr);
    err = udp_bind(udp_raw_pcb, &ipaddr, 53);
//    err = udp_bind(udp_raw_pcb, IP_ANY_TYPE, 53);
    if (err == ERR_OK) {
      udp_recv(udp_raw_pcb, udp_raw_recv, NULL);
    } else {
      /* abort? output diagnostic? */
    }
  } else {
    /* abort? output diagnostic? */
  }
}

#endif /* LWIP_UDP */
