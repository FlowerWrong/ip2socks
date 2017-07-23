#!/bin/bash

SOCKS_SERVER=$1 # SOCKS 服务器的 IP 地址
SOCKS_PORT=1080 # 本地SOCKS 服务器的端口

# $(ip route | awk '/default/ { print $3 }')
GATEWAY_IP=$2 # 家用网关（路由器）的 IP 地址
TUN_NETWORK_DEV=tap0
TUN_NETWORK_PREFIX=172.16.0 # 选一个不冲突的内网 IP 段的前缀

stop_fwd() {
  ip route del 128.0.0.0/1 via "$TUN_NETWORK_PREFIX.2"
  ip route del 0.0.0.0/1 via "$TUN_NETWORK_PREFIX.2"
  for i in $(cat /home/yy/dev/ruby/yyrp/examples/badvpn/china_ip_list/china_ip_list.txt); do
    ip route del "$i" via "$GATEWAY_IP"
  done

  ip route del "$SOCKS_SERVER" via "$GATEWAY_IP"
  ip route add default via "$GATEWAY_IP"
}

stop_fwd
