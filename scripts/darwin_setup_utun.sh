#!/usr/bin/env bash

local_tun_ip=10.0.0.1
remote_tun_ip=10.0.0.1
dns_server=8.8.8.8
# server=$1
intf=utun7
mtu=1500

# get current gateway
orig_gw=$(netstat -nr | grep --color=never '^default' | grep -v 'utun' | sed 's/default *\([0-9\.]*\) .*/\1/' | head -1)
echo $orig_gw
#route add -net $server $orig_gw

# configure IP address and MTU of VPN interface
ifconfig $intf $local_tun_ip $remote_tun_ip mtu $mtu netmask 255.255.255.0 up

# change routing table
#echo changing default route
#route add -net 128.0.0.0 $remote_tun_ip -netmask 128.0.0.0
#route add -net 0.0.0.0 $remote_tun_ip -netmask 128.0.0.0
