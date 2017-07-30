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

  int ret = socks5_auth(sock, conf->remote_dns_server, "53", 1);
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
  int socks_fd = 0;
  struct sockaddr_in socks_proxy_addr;
  int nread;
  char buf[4096];

  socks_proxy_addr.sin_family = AF_INET;
  socks_proxy_addr.sin_addr.s_addr = inet_addr(conf->socks_server);
  socks_proxy_addr.sin_port = htons(atoi(conf->socks_port));

  if ((socks_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("socket failed\n");
    return -1;
  }

  /**
   * socks 5 method request start
   */
  ((socks5_method_req_t *) buf)->ver = SOCKS5_VERSION;
  ((socks5_method_req_t *) buf)->nmethods = 0x01;
  ((socks5_method_req_t *) buf)->methods[0] = 0x00;
  nread = sendto(socks_fd, buf, 3, 0, reinterpret_cast<const sockaddr *>(&socks_proxy_addr),
                 sizeof(socks_proxy_addr));
  if (nread < 0) {
    printf("udp socks 5 method request sendto failed\n");
    return -1;
  }
  nread = recvfrom(socks_fd, buf, 2, 0, reinterpret_cast<sockaddr *>(&socks_proxy_addr),
                   reinterpret_cast<socklen_t *>(sizeof(socks_proxy_addr)));
  if (nread < 0) {
    printf("udp socks 5 method request recvfrom failed\n");
    return -1;
  }

  {
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

    int p = atoi("53");
    socksreq[idx++] = (unsigned char) ((p >> 8) & 0xff); /* PORT MSB */
    socksreq[idx++] = (unsigned char) (p & 0xff);        /* PORT LSB */

    nread = sendto(socks_fd, socksreq, idx, 0, reinterpret_cast<const sockaddr *>(&socks_proxy_addr),
                   sizeof(socks_proxy_addr));
    if (nread < 0) {
      printf("udp socks 5 request sendto failed\n");
      return -1;
    }
    free(saddr_in);

    nread = recvfrom(socks_fd, buf, 10, 0, reinterpret_cast<sockaddr *>(&socks_proxy_addr),
                     reinterpret_cast<socklen_t *>(sizeof(socks_proxy_addr)));
    if (nread < 0) {
      printf("udp socks 5 response recvfrom failed\n");
      return -1;
    }
  }


  nread = sendto(socks_fd, query, len, 0, reinterpret_cast<const sockaddr *>(&socks_proxy_addr),
                 sizeof(socks_proxy_addr));
  if (nread < 0) {
    printf("udp query sendto failed\n");
    return -1;
  }

  buffer->length = recvfrom(socks_fd, buffer->buffer, 4096, 0, reinterpret_cast<sockaddr *>(&socks_proxy_addr),
                            reinterpret_cast<socklen_t *>(sizeof(socks_proxy_addr)));
  if (buffer->length < 0) {
    printf("udp data recvfrom failed\n");
    return -1;
  }

  return socks_fd;
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
