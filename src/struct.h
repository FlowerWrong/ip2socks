//
// Created by king Yang on 2017/7/30.
//

#ifndef EV_STRUCT_H_H
#define EV_STRUCT_H_H

struct Conf {
    char *ip_mode;
    char *socks_server;
    char *socks_port;
    char *remote_dns_server;
    char *gw;
    char *addr;
    char *netmask;
};

struct tuntapif {
    /* Add whatever per-interface state that is needed here. */
    int fd;
};

extern struct Conf *conf;

#endif //EV_STRUCT_H_H
