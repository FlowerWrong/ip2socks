//
// Created by king Yang on 2017/8/20.
//



#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "usocket.h"

int socket_waitfd(socket_t *h, int sw, timeout_t tm) {
  int ret;
  fd_set rfds;
  fd_set wfds;
  fd_set *rp;
  fd_set *wp;
  struct timeval tv;
  struct timeval *tp;

  if (tm == 0) {
    return IO_TIMEOUT;
  }

  do {
    /* must set bits within loop, because select may have modifed them */
    rp = wp = NULL;
    if (sw & WAITFD_R) {
      FD_ZERO(&rfds);
      FD_SET(*h, &rfds);
      rp = &rfds;
    }

    if (sw & WAITFD_W) {
      FD_ZERO(&wfds);
      FD_SET(*h, &wfds);
      wp = &wfds;
    }

    tp = NULL;
    if (tm >= 0.0) {
      tv.tv_sec = (int) tm;
      tv.tv_usec = (int) ((tm - tv.tv_sec) * 1.0e6);
      tp = &tv;
    }
    ret = select(*h + 1, rp, wp, NULL, tp);

  } while (ret == -1 && errno == EINTR);

  if (ret == -1) {
    return errno;
  }

  if (ret == 0) {
    return IO_TIMEOUT;
  }

  if (sw == WAITFD_C && FD_ISSET(*h, &rfds)) {
    return IO_CLOSED;
  }

  return IO_DONE;
}


int socket_select(socket_t *n, fd_set *rfds, fd_set *wfds, fd_set *efds, timeout_t tm) {
  int ret;
  struct timeval tv;

  do {
    tv.tv_sec = (int) tm;
    tv.tv_usec = (int) ((tm - tv.tv_sec) * 1.0e6);

    /* timeout = 0 means no wait */
    ret = select(*n + 1, rfds, wfds, efds, tm >= 0.0 ? &tv : NULL);

  } while (ret < 0 && errno == EINTR);

  return ret;
}

int socket_startup(void) {
  /* instals a handler to ignore sigpipe or it will crash us */
  signal(SIGPIPE, SIG_IGN);
  return 1;
}

int socket_cleanup(void) {
  return 1;
}

int socket_create(socket_t *h, int domain, int type, int protocol) {
  *h = socket(domain, type, protocol);

  if (*h != SOCKET_INVALID) {
    return IO_DONE;
  } else {
    return errno;
  }
}

void socket_destroy(socket_t *h) {
  if (*h != SOCKET_INVALID) {
    socket_setblocking(h);
    close(*h);
    *h = SOCKET_INVALID;
  }
}

void socket_shutdown(socket_t *h, int how) {
  socket_setblocking(h);
  shutdown(*h, how);
  socket_setnonblocking(h);
}

int socket_connect(socket_t *h, sockaddr *addr, socklen_t len, timeout_t tm) {
  int err;

  if (*h == SOCKET_INVALID) {
    return IO_CLOSED;
  }

  do {
    if (connect(*h, addr, len) == 0) {
      return IO_DONE;
    }
  } while ((err = errno) == EINTR);

  /* if connection failed immediately, return error code */
  if (err != EINPROGRESS && err != EAGAIN) {
    return err;
  }

  /* zero timeout case optimization */
  if (tm == 0) {
    return IO_TIMEOUT;
  }

  /* wait until we have the result of the connection attempt or timeout */
  err = socket_waitfd(h, WAITFD_C, tm);
  if (err == IO_CLOSED) {
    if (recv(*h, (char *) &err, 0, 0) == 0) {
      return IO_DONE;
    } else {
      return errno;
    }

  } else {
    return err;
  }
}

int socket_bind(socket_t *h, sockaddr *addr, socklen_t len) {
  int err = IO_DONE;
  int on = 1;

  setsockopt(*h, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  socket_setblocking(h);
  if (bind(*h, addr, len) < 0) {
    err = errno;
  }
  socket_setnonblocking(h);
  return err;
}

int socket_listen(socket_t *h, int backlog) {
  int err = IO_DONE;

  socket_setblocking(h);
  if (listen(*h, backlog)) {
    err = errno;
  }
  socket_setnonblocking(h);
  return err;
}

int socket_accept(socket_t *h, socket_t *pa, sockaddr *addr, socklen_t *len, timeout_t tm) {
  sockaddr daddr;
  socklen_t dlen;
  int err;

  dlen = sizeof(daddr);

  if (*h == SOCKET_INVALID) {
    return IO_CLOSED;
  }

  if (addr == NULL) {
    addr = &daddr;
  }

  if (len == NULL) {
    len = &dlen;
  }

  for (;;) {
    if ((*pa = accept(*h, addr, len)) != SOCKET_INVALID) {
      return IO_DONE;
    }

    err = errno;
    if (err == EINTR) {
      continue;
    }

    if (err != EAGAIN && err != ECONNABORTED) {
      return err;
    }

    if ((err = socket_waitfd(h, WAITFD_R, tm)) != IO_DONE) {
      return err;
    }
  }

  /* can't reach here */
  return IO_UNKNOWN;
}

int socket_send(socket_t *h, const char *data, size_t count, size_t *sent, timeout_t tm) {
  int err;
  long put;

  *sent = 0;

  if (*h == SOCKET_INVALID) {
    return IO_CLOSED;
  }

  /* loop until we send something or we give up on error */
  for (;;) {
    put = (long) send(*h, data, count, 0);
    if (put > 0) {
      *sent = put;
      return IO_DONE;
    }

    err = errno;
    /* send can't really return 0,
     * but EPIPE means the connection was
     * closed
     */
    if (put == 0 || err == EPIPE) {
      return IO_CLOSED;
    }

    /* we call was interrupted, just try again */
    if (err == EINTR) {
      continue;
    }

    /* if failed fatal reason, report error */
    if (err != EAGAIN) {
      return err;
    }

    /* wait until we can send something or we timeout */
    if ((err = socket_waitfd(h, WAITFD_W, tm)) != IO_DONE) {
      return err;
    }
  }

  /* can't reach here */
  return IO_UNKNOWN;
}

int socket_recv(socket_t *h, char *data, size_t count, size_t *got, timeout_t tm) {
  int err;
  long taken;

  *got = 0;

  if (*h == SOCKET_INVALID) {
    return IO_CLOSED;
  }

  for (;;) {
    taken = (long) recv(*h, data, count, 0);
    if (taken > 0) {
      *got = taken;
      return IO_DONE;
    }

    err = errno;
    if (taken == 0) {
      return IO_CLOSED;
    }

    if (err == EINTR) {
      continue;
    }

    if (err != EAGAIN) {
      return err;
    }

    if ((err = socket_waitfd(h, WAITFD_R, tm)) != IO_DONE) {
      return err;
    }
  }

  return IO_UNKNOWN;
}

void socket_setblocking(socket_t *h) {
  int flags = fcntl(*h, F_GETFL, 0);
  flags &= (~(O_NONBLOCK));
  fcntl(*h, F_SETFL, flags);
}

void socket_setnonblocking(socket_t *h) {
  int flags = fcntl(*h, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(*h, F_SETFL, flags);
}

int socket_gethostbyaddr(const char *addr, socklen_t len, struct hostent **hp) {
  *hp = gethostbyaddr(addr, len, AF_INET);
  if (*hp) {
    return IO_DONE;
  } else if (h_errno) {
    return h_errno;
  } else if (errno) {
    return errno;
  } else {
    return IO_UNKNOWN;
  }
}

int socket_gethostbyname(const char *hostname, struct hostent **hp) {
  *hp = gethostbyname(hostname);

  if (*hp) {
    return IO_DONE;
  } else if (h_errno) {
    return h_errno;
  } else if (errno) {
    return errno;
  } else {
    return IO_UNKNOWN;
  }
}

const char *socket_strerror(int err) {
  switch (err) {
    case IO_DONE:
      return NULL;
    case IO_CLOSED:
      return "closed";
    case IO_TIMEOUT:
      return "timeout";
    case HOST_NOT_FOUND:
      return "host not found";
    case EADDRINUSE:
      return "address already in use";
    case EISCONN:
      return "already connected";
    case EACCES:
      return "permission denied";
    case ECONNREFUSED:
      return "connection refused";
    case ECONNABORTED:
      return "closed";
    case ECONNRESET:
      return "closed";
    case ETIMEDOUT:
      return "timeout";
    default:
      return NULL;
  }
}
