/**
 * based on lwip-contrib
 */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory.h>
#include <time.h>
#include <ev.h>
#include "lwip/opt.h"
#include "lwip/udp.h"
#include "lwip/ip.h"

#include "udp_raw.h"
#include "struct.h"
#include "socks5.h"

#if LWIP_UDP

static struct udp_pcb *udp_raw_pcb;
#define container_of(ptr, type, member) ({      \
  const typeof( ((type *)0)->member ) *__mptr = (ptr);  \
  (type *)( (char *)__mptr - offsetof(type,member) );})

struct udp_raw_state {
    ev_io io;
    int socks_tcp_fd;
    u8_t state;
    u8_t retries;
    struct udp_pcb *pcb;
    struct sockaddr_in addr;
    char addr_ip[INET_ADDRSTRLEN];
    ssize_t addr_len;
    const ip_addr_t *udp_addr;
    u16_t udp_port;
};

typedef struct {
    char *buffer;
    ssize_t length;
} response;

int tcp_dns_query(void *query, response *buffer, int len, u16_t dns_port) {
  int sock = socks5_connect(conf->socks_server, conf->socks_port);
  if (sock < 1) {
    printf("socks5 connect failed\n");
    return -1;
  }

  char port[16];
  sprintf(port, "%d", dns_port);

  int ret = socks5_auth(sock, conf->remote_dns_server, port, 0x01, 1);
  if (ret < 0) {
    printf("socks5 auth failed\n");
    return -1;
  }

  // forward dns query
  send(sock, query, len, 0);
  buffer->length = recv(sock, buffer->buffer, 4096, 0);
  return sock;
}


// This callback is called when data is readable on the UDP socket.
static void udp_cb(EV_P_ ev_io *watcher, int revents) {
  struct udp_raw_state *es = container_of(watcher, struct udp_raw_state, io);
  char buff[BUFFER_SIZE];
  ssize_t nread = recvfrom(watcher->fd, buff, BUFFER_SIZE, 0, (struct sockaddr *) (&(es->addr)),
                           reinterpret_cast<socklen_t *>(&es->addr_len));
  if (nread < 0) {
    printf("udp data recvfrom failed\n");
  }

  /* send received packet back to sender */
  ssize_t data_len = nread - 10;
  struct pbuf *socksp = pbuf_alloc(PBUF_TRANSPORT, (uint16_t) data_len, PBUF_RAM);
  memcpy(socksp->payload, buff + 10, (size_t) data_len);

  struct in_addr ip;
  ip.s_addr = inet_addr(es->addr_ip);

  err_t e = udp_sendto(es->pcb, socksp, reinterpret_cast<const ip_addr_t *>(&ip), es->udp_port);
  if (e != 0) {
    printf("udp_sendto %d %s\n", e, lwip_strerr(e));
  }
  /* free the pbuf */
  pbuf_free(socksp);

  // close socks dns socket
  close(watcher->fd);
  ev_io_stop(EV_DEFAULT, watcher);
  close(es->socks_tcp_fd);
  free(es);
}

/**
 * receive callback for a UDP PCB
 * pcb->recv(pcb->recv_arg, pcb, p, ip_current_src_addr(), src_port)
 */
static void
udp_raw_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
             const ip_addr_t *addr, u16_t port) {
  LWIP_UNUSED_ARG(arg);
  if (p == NULL) {
    return;
  }
  upcb->so_options |= SO_REUSEADDR;

  if (strcmp("tcp", conf->dns_mode) == 0 && upcb->remote_fake_port == 53) {
    printf("Redirect dns query to tcp via socks 5\n");
    response *buffer = (response *) malloc(sizeof(response));
    buffer->buffer = static_cast<char *>(malloc(2048));
    char *query;

    pbuf_copy_partial(p, buffer->buffer, p->tot_len, 0);

    query = static_cast<char *>(malloc(p->len + 3));
    query[0] = 0;
    query[1] = (char) p->len;
    memcpy(query + 2, buffer->buffer, p->len);
    int socks_fd = tcp_dns_query(query, buffer, p->len + 2, upcb->remote_fake_port);

    if (buffer->length > 0) {
      struct pbuf *socksp = pbuf_alloc(PBUF_TRANSPORT, (uint16_t) buffer->length - 2, PBUF_RAM);
      memcpy(socksp->payload, buffer->buffer + 2, (size_t) buffer->length - 2);

      udp_sendto(upcb, socksp, addr, port);
      pbuf_free(socksp);

      // close socks dns socket
      close(socks_fd);
      free(buffer->buffer);
      free(buffer);
      free(query);
      pbuf_free(p);
      return;
    }
  }

  printf("Redirect udp to remote via socks 5\n");
  struct udp_raw_state *es;
  LWIP_UNUSED_ARG(arg);
  es = (struct udp_raw_state *) malloc(sizeof(struct udp_raw_state));
  memset(es, 0, sizeof(struct udp_raw_state));
  es->pcb = upcb;
  es->state = 0;
  es->retries = 0;
  es->udp_addr = addr;
  es->udp_port = port;
  inet_ntop(AF_INET, addr, es->addr_ip, INET_ADDRSTRLEN);


  char buf[TCP_WND];
  pbuf_copy_partial(p, buf, p->tot_len, 0);

  int socks_fd = socks5_connect(conf->socks_server, conf->socks_port);
  if (socks_fd < 1) {
    printf("socks5 connect failed\n");
    return;
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
    return;
  };
  if (SOCKS5_VERSION != ((socks5_method_res_t *) buff)->ver || 0x00 != ((socks5_method_res_t *) buff)->method) {
    printf("socks5_method_res_t error\n");
    return;
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

  int pport = atoi(conf->socks_port);
  socksreq[idx++] = (unsigned char) ((pport >> 8) & 0xff); /* PORT MSB */
  socksreq[idx++] = (unsigned char) (pport & 0xff);        /* PORT LSB */

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
    return;
  };
  if (SOCKS5_VERSION != ((socks5_response_t *) buff)->ver) {
    printf("socks 5 response version error\n");
    return;
  }

  char *udp_ip_str = inet_ntoa(*(in_addr *) (&buff[4]));
  int udp_port = ntohs(*(short *) (&buff[8]));

  struct sockaddr_in socks_proxy_addr;
  socks_proxy_addr.sin_family = AF_INET;
  socks_proxy_addr.sin_addr.s_addr = inet_addr(udp_ip_str);
  socks_proxy_addr.sin_port = htons(udp_port);


  unsigned char udp_req[BUFFER_SIZE];
  int udp_index;

  udp_index = 0;
  udp_req[udp_index++] = 0; /* RSV */
  udp_req[udp_index++] = 0; /* RSV */
  udp_req[udp_index++] = 0; /* FRAG */
  udp_req[udp_index++] = 1; /* ATYP: IPv4 = 1 */

  struct sockaddr_in *udp_saddr_in = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));;
  udp_saddr_in->sin_family = AF_INET;
  char remote_fake_ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(upcb->remote_fake_ip), remote_fake_ip_str, INET_ADDRSTRLEN);
  inet_aton(remote_fake_ip_str, &(udp_saddr_in->sin_addr));
  for (int i = 0; i < 4; i++) {
    udp_req[udp_index++] = ((unsigned char *) &udp_saddr_in->sin_addr.s_addr)[i];
  }

  pport = upcb->remote_fake_port;
  udp_req[udp_index++] = (unsigned char) ((pport >> 8) & 0xff); /* PORT MSB */
  udp_req[udp_index++] = (unsigned char) (pport & 0xff);        /* PORT LSB */

  memcpy(udp_req + udp_index, buf, p->tot_len);

  int udp_relay_fd = socket(AF_INET, SOCK_DGRAM, 0);

  sockaddr_in localAddr;
  memset(&localAddr, 0, sizeof(localAddr));
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(0);
  if (bind(udp_relay_fd, (struct sockaddr *) &localAddr, sizeof(localAddr)) < 0) {
    printf("bind failed\n");
    return;
  }
  int addr_len = sizeof(sockaddr_in);
  ssize_t nread = sendto(udp_relay_fd, udp_req, p->tot_len + udp_index, 0, (struct sockaddr *) (&socks_proxy_addr),
                         addr_len);
  if (nread < 0) {
    printf("udp query sendto failed\n");
    return;
  }

  es->addr = socks_proxy_addr;
  es->addr_len = addr_len;
  es->socks_tcp_fd = socks_fd;

  ev_io_init(&(es->io), udp_cb, udp_relay_fd, EV_READ);
  ev_io_start(EV_DEFAULT, &(es->io));
  pbuf_free(p);
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
    // IP_ANY_TYPE
    err = udp_bind(udp_raw_pcb, &ipaddr, 0);
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
