#!/bin/sh

TUN_IP=$1
DIRECT_IP_LIST=./scripts/china_ip_list/china_ip_list.txt

# get current gateway
GATEWAY_IP=$(ip route | awk '/default/ { print $3 }')
echo "origin gateway is '$GATEWAY_IP'"

sysctl -w net.ipv4.ip_forward=1 >> /dev/null

# direct route
chnroutes=$(grep -E "^([0-9]{1,3}\.){3}[0-9]{1,3}" $DIRECT_IP_LIST |\
    sed -e "s/^/route add /" -e "s/$/ via $GATEWAY_IP/")

ip -batch - <<EOF
	$chnroutes
EOF

# route all flow to tun
ip route add 0.0.0.0/1 via "$TUN_IP"
ip route add 128.0.0.0/1 via "$TUN_IP"

# You must add your none proxy domain dns server to direct route
ip route add 114.114.114.114 via $GATEWAY_IP
ip route add 223.5.5.5 via $GATEWAY_IP

# remote server
# TODO just edit it with yours
ip route add 47.75.85.91 via $GATEWAY_IP
