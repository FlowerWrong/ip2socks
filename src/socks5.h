#ifndef LWIP_SOCKS5_H
#define LWIP_SOCKS5_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>

#include "var.h"

typedef struct {
    uint8_t ver;
    uint8_t nmethods;
    uint8_t methods[0];
} socks5_method_req_t;

typedef struct {
    uint8_t ver;
    uint8_t method;
} socks5_method_res_t;

typedef struct {
    uint8_t ver;
    uint8_t cmd;
    uint8_t rsv;
    uint8_t addrtype;
} socks5_request_t;
typedef socks5_request_t socks5_response_t;

int32_t socks5_sockset(int sockfd);

int socks5_connect(const char *proxy_host, const char *proxy_port);

int socks5_auth(int sockfd, const char *server_host, const char *server_port, u_char cmd, int atype);


#endif //LWIP_SOCKS5_H
