//
// Created by king Yang on 2017/8/20.
//

#ifndef DUKTAPE_SOCKET_H
#define DUKTAPE_SOCKET_H


#ifdef __cplusplus
extern "C" {
#endif

#define TARGET_LINUX 1

#if defined(TARGET_WINDOWS)
#define TARGET_DEFINED
#endif

#if defined(TARGET_LINUX)
#define TARGET_DEFINED
#endif

#ifndef TARGET_DEFINED
#error "Define TARGET_WINDOWS or TARGET_LINUX first"
#endif


#ifdef TARGET_WINDOWS
#include <process.h>
#include <WinSock2.h>
#include <windows.h>

typedef int     socklen_t;
typedef SOCKET  socket_t;
typedef double  timeout_t;//seconds
typedef struct  sockaddr sockaddr;

#define WAITFD_R        1                   //recv
#define WAITFD_W        2                   //send
#define WAITFD_E        4                   //error
#define WAITFD_C        (WAITFD_E|WAITFD_W) //connect

#define SOCKET_INVALID (INVALID_SOCKET)
#endif


#ifdef TARGET_LINUX

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

// @unistd.h
// typedef int socklen_t;
typedef int socket_t;
typedef double timeout_t; // seconds
typedef struct sockaddr sockaddr;

#define SD_BOTH     SHUT_RDWR
#define SD_SEND     SHUT_WR
#define SD_RECEIVE  SHUT_RD


#define WAITFD_R        1
#define WAITFD_W        2
#define WAITFD_C        (WAITFD_R|WAITFD_W)

#define SOCKET_INVALID (-1)

#endif


////////////////////////////////////////////////////////////
//  struct sockaddr {
//      unsigned   short   sa_family;
//      char   sa_data[14];
//  };
//
//  struct in_addr {
//      union {
//          struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
//          struct { u_short s_w1,s_w2; } S_un_w;
//          u_long S_addr;
//      } S_un;
//      #define s_addr  S_un.S_addr
//  };
//
//  struct sockaddr_in {
//      short int           sin_family;
//      unsigned short int  sin_port;
//      struct in_addr      sin_addr;
//      unsigned char       sin_zero[8];
//  };
//


////////////////////////////////////////////////////////////
enum {
    IO_DONE = 0,       /* operation completed successfully */
    IO_TIMEOUT = -1,   /* operation timed out */
    IO_CLOSED = -2,    /* the connection has been closed */
    IO_UNKNOWN = -3
};

////////////////////////////////////////
//system dependent initialize interface
int socket_startup(void);

int socket_cleanup(void);

int socket_create(socket_t *ps, int domain, int type, int protocol);

void socket_destroy(socket_t *ps);

// client (default sync mode)
int socket_connect(socket_t *ps, sockaddr *addr, socklen_t addr_len, timeout_t tm);

// server (default async mode)
int socket_bind(socket_t *ps, sockaddr *addr, socklen_t addr_len);

int socket_listen(socket_t *ps, int backlog);

int socket_accept(socket_t *ps, socket_t *pa, sockaddr *addr,
                  socklen_t *addr_len, timeout_t tm);

// send recv
int socket_send(socket_t *ps, const char *data, size_t count,
                size_t *sent, timeout_t tm);

int socket_recv(socket_t *ps, char *data, size_t count, size_t *got, timeout_t tm);

const char *socket_ioerror(socket_t *ps, int err);

// DNS: hostname -> ip
//rDNS: ip -> hostname
//
//this two method socket_get_hostxxx is rDNS
//
//  struct hostent{
//      char * h_name;
//      char ** h_aliases;
//      short h_addrtype;
//      short h_length;
//      char ** h_addr_list;
//  };
//
//  struct hostent* lpHostEnt
//  in_addr ina;
//  ina.S_un.S_addr = inet_addr("127.0.0.1");
//  lpHostEnt = gethostbyaddr((char*)&ina.S_un.S_addr, 4, AF_INET);
int socket_gethostbyaddr(const char *ipaddr, socklen_t len, struct hostent **hp);

// hostname: "www.baidu.com"
int socket_gethostbyname(const char *hostname, struct hostent **hp);

////////////////////////////////////////
// async mode
void socket_setnonblocking(socket_t *ps);

void socket_setblocking(socket_t *ps);

int socket_waitfd(socket_t *ps, int sw, timeout_t tm);

int socket_select(socket_t *n, fd_set *rfds, fd_set *wfds, fd_set *efds, timeout_t tm);

void socket_shutdown(socket_t *ps, int how);

const char *socket_strerror(int err);


#ifdef __cplusplus
}
#endif

#endif //DUKTAPE_SOCKET_H
