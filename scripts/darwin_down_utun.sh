#!/bin/sh

TUN_IP=$1
DIRECT_IP_LIST=./scripts/china_ip_list/china_ip_list.txt

# get current gateway
GATEWAY_IP=$(netstat -nr | grep --color=never '^default' | grep -v 'utun' | sed 's/default *\([0-9\.]*\) .*/\1/' | head -1)
echo "origin gateway is '$GATEWAY_IP'"

# direct route
if [ -n "$DIRECT_IP_LIST" ]
then
    for i in $(cat $DIRECT_IP_LIST); do
        route delete -net $i $GATEWAY_IP
    done
fi

# route all flow to tun
route delete -net 128.0.0.0 $TUN_IP -netmask 128.0.0.0
route delete -net 0.0.0.0 $TUN_IP -netmask 128.0.0.0

# You must add your none proxy domain dns server to direct route
route delete -host 114.114.114.114 $GATEWAY_IP
route delete -host 223.5.5.5 $GATEWAY_IP

# remote server
# TODO just edit it with yours
route delete -host 47.90.32.252 $GATEWAY_IP
