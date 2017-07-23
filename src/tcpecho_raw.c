#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "socks5.h"
#include "ev.h"

#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "tcpecho_raw.h"

#if LWIP_TCP && LWIP_CALLBACK_API

static struct tcp_pcb *tcpecho_raw_pcb;

#define container_of(ptr, type, member) ({      \
  const typeof( ((type *)0)->member ) *__mptr = (ptr);  \
  (type *)( (char *)__mptr - offsetof(type,member) );})

enum tcpecho_raw_states {
    ES_NONE = 0,
    ES_ACCEPTED,
    ES_RECEIVED,
    ES_CLOSING
};

struct tcpecho_raw_state {
    ev_io io;
    u8_t state;
    u8_t retries;
    struct tcp_pcb *pcb;
    /* pbuf (chain) to recycle */
    // struct pbuf *p;
    int socks_fd;
    uint8_t buf[TCP_WND];
    size_t buf_used;
    uint8_t socks_buf[TCP_WND];
    size_t socks_buf_used;
};

static void tcpecho_raw_send(struct tcp_pcb *tpcb, struct tcpecho_raw_state *es);


static void
tcpecho_raw_free(struct tcpecho_raw_state *es) {
  es->pcb = NULL;

  mem_free(es);
}

static void
tcpecho_raw_close(struct tcp_pcb *tpcb, struct tcpecho_raw_state *es) {
  if (tpcb != NULL) {
    tcp_arg(tpcb, NULL);
    tcp_sent(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_err(tpcb, NULL);
    tcp_poll(tpcb, NULL, 0);
  }

  if (tpcb != NULL) {
    tcp_close(tpcb);
  }

  if (es != NULL) {
    if (es->socks_fd > 0) {
      printf("<---------------------------------- ev io fd %d to be close\n", es->socks_fd);
      if (&(es->io) != NULL) {
        ev_io_stop(EV_DEFAULT, &(es->io));
        close(es->socks_fd);
      }
    }

    tcpecho_raw_free(es);
  }
}

static void
tcpecho_raw_send(struct tcp_pcb *tpcb, struct tcpecho_raw_state *es) {
  if (es->buf_used > 0) {
    ssize_t ret = send(es->socks_fd, es->buf, es->buf_used, 0);

    if (ret > 0) {
      u16_t plen = es->buf_used;

      es->buf_used = 0;

      /* we can read more data now */
      tcp_recved(tpcb, plen);
    } else {
      printf("<-------------------------------------- send to local failed %ld\n", ret);
    }
  }
}

static void
tcpecho_raw_error(void *arg, err_t err) {
  struct tcpecho_raw_state *es;

  LWIP_UNUSED_ARG(err);

  es = (struct tcpecho_raw_state *) arg;

  if (es != NULL) {
    tcpecho_raw_free(es);
  }
}

static err_t
tcpecho_raw_poll(void *arg, struct tcp_pcb *tpcb) {
  err_t ret_err;
  struct tcpecho_raw_state *es;

  es = (struct tcpecho_raw_state *) arg;
  if (es != NULL) {
    if (es->buf_used > 0) {
      /* there is a remaining pbuf (chain)  */
      tcpecho_raw_send(tpcb, es);
    } else {
      /* no remaining pbuf (chain)  */
      if (es->state == ES_CLOSING) {
        tcpecho_raw_close(tpcb, es);
      }
    }
    ret_err = ERR_OK;
  } else {
    /* nothing to be done */
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

static err_t
tcpecho_raw_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
  struct tcpecho_raw_state *es;

  LWIP_UNUSED_ARG(len);

  es = (struct tcpecho_raw_state *) arg;
  es->retries = 0;

  if (es->buf_used > 0) {
    /* still got pbufs to send */
    tcp_sent(tpcb, tcpecho_raw_sent);
    tcpecho_raw_send(tpcb, es);
  } else {
    /* no more pbufs to send */
    if (es->state == ES_CLOSING) {
      tcpecho_raw_close(tpcb, es);
    }
  }
  return ERR_OK;
}

static err_t
tcpecho_raw_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
  struct tcpecho_raw_state *es;
  err_t ret_err;

  LWIP_ASSERT("arg != NULL", arg != NULL);
  es = (struct tcpecho_raw_state *) arg;
  if (p == NULL) {
    /* remote host closed connection */
    es->state = ES_CLOSING;
    if (es->buf_used <= 0) {
      /* we're done sending, close it */
      tcpecho_raw_close(tpcb, es);
    } else {
      /* we're not done yet */
      tcpecho_raw_send(tpcb, es);
    }
    ret_err = ERR_OK;
  } else if (err != ERR_OK) {
    /* cleanup, for unknown reason */
    if (p != NULL) {
      pbuf_free(p);
    }
    ret_err = err;
  } else if (es->state == ES_ACCEPTED) {
    /* first data chunk in p->payload */
    es->state = ES_RECEIVED;

    // check if we have enough buffer
    if (p->tot_len > sizeof(es->buf) - es->buf_used) {
      printf("------------------------------> no buffer for data !?!\n");
      return ERR_MEM;
    }
    pbuf_copy_partial(p, es->buf + es->buf_used, p->tot_len, 0);
    es->buf_used += p->tot_len;

    tcpecho_raw_send(tpcb, es);
    ret_err = ERR_OK;
  } else if (es->state == ES_RECEIVED) {
    // check if we have enough buffer
    if (p->tot_len > sizeof(es->buf) - es->buf_used) {
      printf("no buffer for data !?!\n");
      return ERR_MEM;
    }
    /* read some more data */
    pbuf_copy_partial(p, es->buf + es->buf_used, p->tot_len, 0);
    es->buf_used += p->tot_len;
    tcpecho_raw_send(tpcb, es);

    ret_err = ERR_OK;
  } else {
    /* unkown es->state, trash data  */
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

static void free_all(struct ev_loop *loop, ev_io *watcher, struct tcpecho_raw_state *es, struct tcp_pcb *pcb) {
  tcpecho_raw_close(pcb, es);
}


static void read_cb(struct ev_loop *loop, ev_io *watcher, int revents) {
  struct tcpecho_raw_state *es = container_of(watcher, struct tcpecho_raw_state, io);
  struct tcp_pcb *pcb = es->pcb;

  char buffer[BUFFER_SIZE];
  ssize_t nreads;

  nreads = recv(watcher->fd, buffer, BUFFER_SIZE, 0);
  if (nreads < 0) {
    printf("<---------------------------------- read error [%d].\n", errno);
    free_all(loop, watcher, es, pcb);
    return;
  }

  if (0 == nreads) {
    printf("<---------------------------------- close socks fd %d.\n", watcher->fd);
    free_all(loop, watcher, es, pcb);
    return;
  }

  // https://github.com/ambrop72/badvpn/blob/master/tun2socks/tun2socks.c#L1671
  // write to tcp pcb
  printf("read %ld bytes tcp_sndbuf(pcb) is %d\n", nreads, tcp_sndbuf(pcb));
  if (nreads <= tcp_sndbuf(pcb)) {
    err_t wr_err = ERR_OK;
    wr_err = tcp_write(pcb, (void *) buffer, nreads, TCP_WRITE_FLAG_COPY);
    if (wr_err == ERR_OK) {
    } else if (ERR_MEM == wr_err) {
      printf("<---------------------------------- out of memory\n");

      if (es->retries > 16) {
        free_all(loop, watcher, es, pcb);
        return;
      }
      es->retries += 1;
      tcp_write(pcb, (void *) buffer, nreads, TCP_WRITE_FLAG_COPY);
    } else {
      printf("<---------------------------------- tcp_write wr_wrr is %d\n", wr_err);

      free_all(loop, watcher, es, pcb);
      return;
    }
    wr_err = tcp_output(pcb);
    if (wr_err != ERR_OK) {
      printf("<---------------------------------- tcp_output wr_wrr is %d\n", wr_err);
      free_all(loop, watcher, es, pcb);
      return;
    }
  } else {
    // TODO
    printf("<--------------------- ========== nreads %ld > tcp_sndbuf(pcb) %d\n", nreads, tcp_sndbuf(pcb));
  }
  return;
}

static err_t
tcpecho_raw_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
  err_t ret_err;
  struct tcpecho_raw_state *es;

  LWIP_UNUSED_ARG(arg);
  if ((err != ERR_OK) || (newpcb == NULL)) {
    return ERR_VAL;
  }

  /* Unless this pcb should have NORMAL priority, set its priority now.
     When running out of pcbs, low priority pcbs can be aborted to create
     new pcbs of higher priority. */
  tcp_setprio(newpcb, TCP_PRIO_MIN);

  /**
   * local ip local port <-> remote ip remote port
   */
  char localip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(newpcb->local_ip), localip_str, INET_ADDRSTRLEN);
  char remoteip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(newpcb->remote_ip), remoteip_str, INET_ADDRSTRLEN);

  // flow 119.23.211.95:80 <-> 172.16.0.1:53536
  printf("<--------------------- flow %s:%d <-> %s:%d\n", localip_str, newpcb->local_port, remoteip_str,
         newpcb->remote_port);

  /**
   * socks 5
   */
  int socks_fd = 0;
  char *ip = "127.0.0.1";

  socks_fd = socks5_connect(ip, "1080");
  if (socks_fd < 1) {
    printf("socks5 connect failed\n");
    return -1;
  }

  char port[64];
  sprintf(port, "%d", newpcb->local_port);

  int ret = socks5_auth(socks_fd, localip_str, port, 1);
  if (ret < 0) {
    printf("socks5 auth error\n");
    return -1;
  }
  printf("socks 5 auth success fd is %d\n", socks_fd);

  es = (struct tcpecho_raw_state *) mem_malloc(sizeof(struct tcpecho_raw_state));

  if (es != NULL) {
    es->state = ES_ACCEPTED;
    es->pcb = newpcb;
    es->retries = 0;
    es->buf_used = 0;
    es->socks_buf_used = 0;
    es->socks_fd = socks_fd;

    ev_io_init(&(es->io), read_cb, socks_fd, EV_READ);
    ev_io_start(EV_DEFAULT, &(es->io));

    /* pass newly allocated es to our callbacks */
    tcp_arg(newpcb, es);
    tcp_recv(newpcb, tcpecho_raw_recv);
    tcp_err(newpcb, tcpecho_raw_error);
    tcp_poll(newpcb, tcpecho_raw_poll, 0);
    tcp_sent(newpcb, tcpecho_raw_sent);
    ret_err = ERR_OK;
  } else {
    ret_err = ERR_MEM;
  }
  return ret_err;
}

void
tcpecho_raw_init(void) {
  tcpecho_raw_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (tcpecho_raw_pcb != NULL) {
    err_t err;

    err = tcp_bind(tcpecho_raw_pcb, IP_ANY_TYPE, 0);
    if (err == ERR_OK) {
      tcpecho_raw_pcb = tcp_listen(tcpecho_raw_pcb);
      tcp_accept(tcpecho_raw_pcb, tcpecho_raw_accept);
    } else {
      /* abort? output diagnostic? */
    }
  } else {
    /* abort? output diagnostic? */
  }
}

#endif /* LWIP_TCP && LWIP_CALLBACK_API */
