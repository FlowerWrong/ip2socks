/**
 * based on lwip-contrib
 */
#ifndef LWIP_TCP_RAW_H
#define LWIP_TCP_RAW_H

enum tcp_raw_states {
    ES_NONE = 0,
    ES_ACCEPTED,
    ES_RECEIVED,
    ES_CLOSING
};

typedef struct timer_ctx {
    ev_timer watcher;
    struct tcp_raw_state *raw_state;
} timer_ctx;

typedef struct tcp_raw_state {
    ev_io io;
    struct timer_ctx *timeout_ctx;
    struct timer_ctx *block_ctx;
    u8_t state;
    u8_t retries;
    struct tcp_pcb *pcb;
    int socks_fd;
    std::string buf;
    u16_t buf_used;
    std::string socks_buf;
    u16_t socks_buf_used;
    int lwip_blocked;
} tcp_raw_state;

void tcp_raw_init(void);

#endif /* LWIP_TCP_RAW_H */
