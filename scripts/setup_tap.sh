#!/bin/bash

SOCKS_SERVER=$1 # SOCKS 服务器的 IP 地址
SOCKS_PORT=1080 # 本地SOCKS 服务器的端口

# $(ip route | awk '/default/ { print $3 }')
GATEWAY_IP=$2 # 家用网关（路由器）的 IP 地址
TUN_NETWORK_DEV=tap0
TUN_NETWORK_PREFIX=172.16.0 # 选一个不冲突的内网 IP 段的前缀

start_fwd() {
  ip route del default via "$GATEWAY_IP"
  ip route add "$SOCKS_SERVER" via "$GATEWAY_IP"
  # DNS
  # 国内网段走家用网关（路由器）的 IP 地址
  for i in $(cat '/home/yy/dev/ruby/yyrp/examples/badvpn/china_ip_list/china_ip_list.txt'); do
    ip route add "$i" via "$GATEWAY_IP"
  done

  # 将默认网关设为虚拟网卡的IP地址
  ip route add 0.0.0.0/1 via "$TUN_NETWORK_PREFIX.2"
  ip route add 128.0.0.0/1 via "$TUN_NETWORK_PREFIX.2"
}

start_fwd
