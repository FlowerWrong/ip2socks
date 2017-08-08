/**
 * based on lwip-contrib
 */
#ifndef LWIP_UDP_RAW_H
#define LWIP_UDP_RAW_H

#include "lwip/ip6.h"
#include "ev.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory.h>
#include <time.h>
#include <regex.h>


typedef struct udp_timer_ctx {
    ev_timer watcher;
    struct udp_raw_state *raw_state;
} udp_timer_ctx;

struct udp_raw_state {
    ev_io io;
    struct udp_timer_ctx *timeout_ctx;
    int socks_tcp_fd; // just for udp relay via socks5
    u8_t state;
    u8_t retries;
    struct udp_pcb *pcb;
    struct sockaddr_in addr; // recvfrom addr
    char addr_ip[INET_ADDRSTRLEN]; // origin sendto ip address
    ssize_t addr_len;
    u16_t udp_port; // origin sendto port
};

typedef struct {
    char *buffer;
    ssize_t length;
} response;

void udp_raw_init(void);

#endif /* LWIP_UDP_RAW_H */
