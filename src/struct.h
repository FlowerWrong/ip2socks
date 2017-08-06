//
// Created by king Yang on 2017/7/30.
//

#ifndef EV_STRUCT_H_H
#define EV_STRUCT_H_H

#include <vector>
#include <iostream>

struct Conf {
    char *ip_mode;
    char *dns_mode;
    char *socks_server;
    char *socks_port;
    char *remote_dns_server;
    char *remote_dns_port;
    char *local_dns_port;
    char *custom_domian_server_file;
    char *gw;
    char *addr;
    char *netmask;
    std::vector<std::vector<std::string>> domains;
};

struct tuntapif {
    /* Add whatever per-interface state that is needed here. */
    int fd;
};

extern struct Conf *conf;

#endif //EV_STRUCT_H_H
