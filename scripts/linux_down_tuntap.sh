#!/bin/sh

TUN_IP=$1
DIRECT_IP_LIST=./scripts/china_ip_list/china_ip_list.txt

# get current gateway
GATEWAY_IP=$(ip route | awk '/default/ { print $3 }')
echo "origin gateway is '$GATEWAY_IP'"

# direct route
chnroutes=$(grep -E "^([0-9]{1,3}\.){3}[0-9]{1,3}" $DIRECT_IP_LIST |\
    sed -e "s/^/route del /" -e "s/$/ via $GATEWAY_IP/")

ip -batch - <<EOF
	$chnroutes
EOF

# route all flow to tun
ip route del 0.0.0.0/1 via "$TUN_IP"
ip route del 128.0.0.0/1 via "$TUN_IP"

# remote server
# TODO just edit it with yours
ip route del 47.90.32.252 via $GATEWAY_IP
