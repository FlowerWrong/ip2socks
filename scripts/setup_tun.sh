#!/bin/bash

SOCKS_SERVER=$1 # SOCKS 服务器的 IP 地址
SOCKS_PORT=1080 # 本地SOCKS 服务器的端口

# $(ip route | awk '/default/ { print $3 }')
GATEWAY_IP=$2 # 家用网关（路由器）的 IP 地址
TUN_NETWORK_DEV=tun0
TUN_NETWORK_PREFIX=10.0.0 # 选一个不冲突的内网 IP 段的前缀

start_fwd() {
    ip addr add "$TUN_NETWORK_PREFIX.1/24" dev "$TUN_NETWORK_DEV"
    ip link set "$TUN_NETWORK_DEV" up
    ip route del default via "$GATEWAY_IP"
    ip route add "$SOCKS_SERVER" via "$GATEWAY_IP"

    # DNS
    ip route add "114.114.114.114" via "$GATEWAY_IP"

    # 国内网段走家用网关（路由器）的 IP 地址
#    for i in $(cat '/home/yy/dev/ruby/yyrp/examples/badvpn/china_ip_list/china_ip_list.txt'); do
#        ip route add "$i" via "$GATEWAY_IP"
#    done

    # 将默认网关设为虚拟网卡的IP地址
    ip route add 0.0.0.0/1 via "$TUN_NETWORK_PREFIX.1"
    ip route add 128.0.0.0/1 via "$TUN_NETWORK_PREFIX.1"
}

start_fwd
