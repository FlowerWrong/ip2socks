#!/bin/sh

SOCKS_SERVER=$1 # SOCKS 服务器的 IP 地址
SOCKS_PORT=1080 # 本地SOCKS 服务器的端口

local_tun_ip=10.0.0.1
remote_tun_ip=10.0.0.1
intf=utun7
mtu=1500

CHINA_IP=$2

# get current gateway
orig_gw=$(netstat -nr | grep --color=never '^default' | grep -v 'utun' | sed 's/default *\([0-9\.]*\) .*/\1/' | head -1)
echo $orig_gw

# configure IP address and MTU of VPN interface
ifconfig $intf $local_tun_ip $remote_tun_ip mtu $mtu netmask 255.255.255.0 up
echo $SOCKS_SERVER
echo $orig_gw
route add -net $SOCKS_SERVER $orig_gw

# 国内网段走家用网关（路由器）的 IP 地址
for i in $(cat $CHINA_IP); do
    route add -net $i $orig_gw
done

# change routing table
route add -net 128.0.0.0 $remote_tun_ip -netmask 128.0.0.0
route add -net 0.0.0.0 $remote_tun_ip -netmask 128.0.0.0
