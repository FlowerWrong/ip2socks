//
// Created by yy on 17-7-7.
//
#include <fcntl.h>
#include "socks5.h"

int32_t socks5_sockset(int sockfd) {
  struct timeval tmo = {0};
  int opt = 1;

  tmo.tv_sec = 2;
  tmo.tv_usec = 0;
  if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tmo, sizeof(tmo)) ||
      -1 == setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &tmo, sizeof(tmo))) {
    printf("setsockopt error.\n");
    return -1;
  }

#ifdef SO_NOSIGPIPE
  setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif

  if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (uint *) &opt, sizeof(opt))) {
    printf("setsockopt SO_REUSEADDR fail.\n");
    return -1;
  }

  return 0;
}

int socks5_connect(const char *proxy_host, const char *proxy_port) {
  int socks_fd = 0;
  struct sockaddr_in socks_proxy_addr;

  socks_proxy_addr.sin_family = AF_INET;
  socks_proxy_addr.sin_addr.s_addr = inet_addr(proxy_host);
  socks_proxy_addr.sin_port = htons(atoi(proxy_port));

  if ((socks_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("socket failed\n");
    return -1;
  }
  // setnonblocking(socks_fd);
  socks5_sockset(socks_fd);
  if (0 > connect(socks_fd, (struct sockaddr *) &socks_proxy_addr, sizeof(socks_proxy_addr))) {
    printf("connect failed\n");
    return -1;
  }

  return socks_fd;
}

int socks5_auth(int sockfd, const char *server_host, const char *server_port, int atype) {
  char buff[BUFFER_SIZE];
  /**
   * socks 5 method request start
   */
  ((socks5_method_req_t *) buff)->ver = SOCKS5_VERSION;
  ((socks5_method_req_t *) buff)->nmethods = 0x01;
  ((socks5_method_req_t *) buff)->methods[0] = 0x00;

  send(sockfd, buff, 3, 0);

  // VERSION and METHODS
  if (-1 == recv(sockfd, buff, 2, 0)) {
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
   * https://github.com/curl/curl/blob/ce2c3ebda20919fe636e675f219ae387e386f508/lib/socks.c#L353
   */
  if (atype == 1) {
    unsigned char socksreq[600];
    int idx;

    idx = 0;
    socksreq[idx++] = 5; /* version */
    socksreq[idx++] = 1;
    socksreq[idx++] = 0;
    socksreq[idx++] = 1; /* ATYP: IPv4 = 1 */

    struct sockaddr_in *saddr_in = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));;
    saddr_in->sin_family = AF_INET;
    inet_aton(server_host, &(saddr_in->sin_addr));
    for (int i = 0; i < 4; i++) {
      socksreq[idx++] = ((unsigned char *) &saddr_in->sin_addr.s_addr)[i];
    }

    int p = atoi(server_port);
    socksreq[idx++] = (unsigned char) ((p >> 8) & 0xff); /* PORT MSB */
    socksreq[idx++] = (unsigned char) (p & 0xff);        /* PORT LSB */

    send(sockfd, (char *) socksreq, idx, 0);
    free(saddr_in);
  } else if (atype == 3) {
    char *buffer, *temp;
    u_char v = 0x05, cmd = 0x01, rsv = 0x00, atyp = 0x03;

    const char host_len = (char) strlen(server_host);
    uint16_t port = htons(atoi(server_port));
    buffer = static_cast<char *>(malloc(strlen(server_host) + 7));
    temp = buffer;
    /* Assemble the request packet */
    (void) memcpy(temp, &v, sizeof(v));
    temp += sizeof(v);
    (void) memcpy(temp, &cmd, sizeof(cmd));
    temp += sizeof(cmd);
    (void) memcpy(temp, &rsv, sizeof(rsv));
    temp += sizeof(rsv);
    (void) memcpy(temp, &atyp, sizeof(atyp));
    temp += sizeof(atyp);
    (void) memcpy(temp, &host_len, sizeof(host_len));
    temp += sizeof(host_len);
    (void) memcpy(temp, server_host, strlen(server_host));
    temp += strlen(server_host);
    (void) memcpy(temp, &port, sizeof(&port));
    temp += sizeof(&port);
    send(sockfd, buffer, temp - buffer, 0);
    free(buffer);
  } else if (atype == 4) {
    // ipv6 TODO
  }
  /**
   * socks 5 request end
   */

  /**
   * socks 5 response
   */
  if (-1 == recv(sockfd, buff, 10, 0)) {
    printf("recv socks 5 response error\n");
    return -1;
  };

  if (SOCKS5_VERSION != ((socks5_response_t *) buff)->ver) {
    printf("socks 5 response version error\n");
    return -1;
  }

  return 0;
}
