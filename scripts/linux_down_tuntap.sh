#!/bin/sh

TUN_IP=$1
DIRECT_IP_LIST=./scripts/china_ip_list/china_ip_list.txt

# get current gateway
GATEWAY_IP=$(ip route | awk '/default/ { print $3 }')
echo "origin gateway is '$GATEWAY_IP'"

# direct route
if [ -n "$DIRECT_IP_LIST" ]
then
    for i in $(cat "$DIRECT_IP_LIST"); do
        ip route del "$i" via "$GATEWAY_IP"
    done
fi

# route all flow to tun
ip route del 0.0.0.0/1 via "$TUN_IP"
ip route del 128.0.0.0/1 via "$TUN_IP"

# route dns flow to tun, and redirect to tcp dns server
# TODO just edit it with yours
ip route del 114.114.114.114 via "$TUN_IP"
ip route del 223.5.5.5 via "$TUN_IP"

# remote server
# TODO just edit it with yours
ip route del 47.90.32.252 via $GATEWAY_IP
