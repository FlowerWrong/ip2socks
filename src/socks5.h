//
// Created by yy on 17-7-7.
//

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

#define BUFFER_SIZE 1514

// socks5 version
#define SOCKS5_VERSION 0x05
// socks5 command
#define SOCKS5_CMD_CONNECT 0x01
#define SOCKS5_CMD_BIND 0x02
#define SOCKS5_CMD_UDPASSOCIATE 0x03

// socks5 address type
#define SOSKC5_ADDRTYPE_IPV4 0x01
#define SOSKC5_ADDRTYPE_DOMAIN 0x03
#define SOSKC5_ADDRTYPE_IPV6 0x04

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

int socks5_auth(int sockfd, const char *server_host, const char *server_port, int atype);


#endif //LWIP_SOCKS5_H
