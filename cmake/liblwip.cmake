cmake_minimum_required(VERSION 2.8)
project(lwip C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -g -Wall -DLWIP_DEBUG")

set(LWIPDIR lwip/src)
set(LWIPARCH lwip-contrib/ports/unix/port)


include_directories(
    ${LWIPDIR}/include
    ${LWIPARCH}/include
)

set(LWIP_SOURCE_FILES
    # COREFILES
    ${LWIPDIR}/core/init.c
    ${LWIPDIR}/core/def.c
    ${LWIPDIR}/core/dns.c
    ${LWIPDIR}/core/inet_chksum.c
    ${LWIPDIR}/core/ip.c
    ${LWIPDIR}/core/mem.c
    ${LWIPDIR}/core/memp.c
    ${LWIPDIR}/core/netif.c
    ${LWIPDIR}/core/pbuf.c
    ${LWIPDIR}/core/raw.c
    ${LWIPDIR}/core/stats.c
    ${LWIPDIR}/core/sys.c
    ${LWIPDIR}/core/tcp.c
    ${LWIPDIR}/core/tcp_in.c
    ${LWIPDIR}/core/tcp_out.c
    ${LWIPDIR}/core/timeouts.c
    ${LWIPDIR}/core/udp.c


    # CORE4FILES
    ${LWIPDIR}/core/ipv4/autoip.c
    ${LWIPDIR}/core/ipv4/dhcp.c
    ${LWIPDIR}/core/ipv4/etharp.c
    ${LWIPDIR}/core/ipv4/icmp.c
    ${LWIPDIR}/core/ipv4/igmp.c
    ${LWIPDIR}/core/ipv4/ip4_frag.c
    ${LWIPDIR}/core/ipv4/ip4.c
    ${LWIPDIR}/core/ipv4/ip4_addr.c


    # CORE6FILES
    ${LWIPDIR}/core/ipv6/dhcp6.c
    ${LWIPDIR}/core/ipv6/ethip6.c
    ${LWIPDIR}/core/ipv6/icmp6.c
    ${LWIPDIR}/core/ipv6/inet6.c
    ${LWIPDIR}/core/ipv6/ip6.c
    ${LWIPDIR}/core/ipv6/ip6_addr.c
    ${LWIPDIR}/core/ipv6/ip6_frag.c
    ${LWIPDIR}/core/ipv6/mld6.c
    ${LWIPDIR}/core/ipv6/nd6.c


    # APIFILES: The files which implement the sequential and socket APIs.
    ${LWIPDIR}/api/api_lib.c
    ${LWIPDIR}/api/api_msg.c
    ${LWIPDIR}/api/err.c
    ${LWIPDIR}/api/netbuf.c
    ${LWIPDIR}/api/netdb.c
    ${LWIPDIR}/api/netifapi.c
    ${LWIPDIR}/api/sockets.c
    ${LWIPDIR}/api/tcpip.c


    # NETIFFILES: Files implementing various generic network interface functions
    ${LWIPDIR}/netif/ethernet.c
    #    ${LWIPDIR}/netif/slipif.c # OSX compile will be break


    # SIXLOWPAN: 6LoWPAN
    ${LWIPDIR}/netif/lowpan6.c


    # PPPFILES: PPP
    #    ${LWIPDIR}/netif/ppp/auth.c
    #    ${LWIPDIR}/netif/ppp/ccp.c
    #    ${LWIPDIR}/netif/ppp/chap-md5.c
    #    ${LWIPDIR}/netif/ppp/chap_ms.c
    #    ${LWIPDIR}/netif/ppp/chap-new.c
    #    ${LWIPDIR}/netif/ppp/demand.c
    #    ${LWIPDIR}/netif/ppp/eap.c
    #    ${LWIPDIR}/netif/ppp/ecp.c
    #    ${LWIPDIR}/netif/ppp/eui64.c
    #    ${LWIPDIR}/netif/ppp/fsm.c
    #    ${LWIPDIR}/netif/ppp/ipcp.c
    #    ${LWIPDIR}/netif/ppp/ipv6cp.c
    #    ${LWIPDIR}/netif/ppp/lcp.c
    #    ${LWIPDIR}/netif/ppp/magic.c
    #    ${LWIPDIR}/netif/ppp/mppe.c
    #    ${LWIPDIR}/netif/ppp/multilink.c
    #    ${LWIPDIR}/netif/ppp/ppp.c
    #    ${LWIPDIR}/netif/ppp/pppapi.c
    #    ${LWIPDIR}/netif/ppp/pppcrypt.c
    #    ${LWIPDIR}/netif/ppp/pppoe.c
    #    ${LWIPDIR}/netif/ppp/pppol2tp.c
    #    ${LWIPDIR}/netif/ppp/pppos.c
    #    ${LWIPDIR}/netif/ppp/upap.c
    #    ${LWIPDIR}/netif/ppp/utils.c
    #    ${LWIPDIR}/netif/ppp/vj.c
    #    ${LWIPDIR}/netif/ppp/polarssl/arc4.c
    #    ${LWIPDIR}/netif/ppp/polarssl/des.c
    #    ${LWIPDIR}/netif/ppp/polarssl/md4.c
    #    ${LWIPDIR}/netif/ppp/polarssl/md5.c
    #    ${LWIPDIR}/netif/ppp/polarssl/sha1.c


    # SNMPFILES: SNMPv2c agent
    #    ${LWIPDIR}/apps/snmp/snmp_asn1.c
    #    ${LWIPDIR}/apps/snmp/snmp_core.c
    #    ${LWIPDIR}/apps/snmp/snmp_mib2.c
    #    ${LWIPDIR}/apps/snmp/snmp_mib2_icmp.c
    #    ${LWIPDIR}/apps/snmp/snmp_mib2_interfaces.c
    #    ${LWIPDIR}/apps/snmp/snmp_mib2_ip.c
    #    ${LWIPDIR}/apps/snmp/snmp_mib2_snmp.c
    #    ${LWIPDIR}/apps/snmp/snmp_mib2_system.c
    #    ${LWIPDIR}/apps/snmp/snmp_mib2_tcp.c
    #    ${LWIPDIR}/apps/snmp/snmp_mib2_udp.c
    #    ${LWIPDIR}/apps/snmp/snmp_msg.c
    #    ${LWIPDIR}/apps/snmp/snmpv3.c
    #    ${LWIPDIR}/apps/snmp/snmp_netconn.c
    #    ${LWIPDIR}/apps/snmp/snmp_pbuf_stream.c
    #    ${LWIPDIR}/apps/snmp/snmp_raw.c
    #    ${LWIPDIR}/apps/snmp/snmp_scalar.c
    #    ${LWIPDIR}/apps/snmp/snmp_table.c
    #    ${LWIPDIR}/apps/snmp/snmp_threadsync.c
    #    ${LWIPDIR}/apps/snmp/snmp_traps.c
    #    ${LWIPDIR}/apps/snmp/snmpv3_mbedtls.c
    #    ${LWIPDIR}/apps/snmp/snmpv3_dummy.c


    #    # HTTPDFILES: HTTP server
    #    ${LWIPDIR}/apps/httpd/fs.c
    #    ${LWIPDIR}/apps/httpd/httpd.c
    #
    #
    #    # LWIPERFFILES: IPERF server
    #    ${LWIPDIR}/apps/lwiperf/lwiperf.c
    #
    #
    #    # SNTPFILES: SNTP client
    #    ${LWIPDIR}/apps/sntp/sntp.c
    #
    #
    #    # MDNSFILES: MDNS responder
    #    ${LWIPDIR}/apps/mdns/mdns.c
    #
    #
    #    # NETBIOSNSFILES: NetBIOS name server
    #    ${LWIPDIR}/apps/netbiosns/netbiosns.c
    #
    #
    #    # TFTPFILES: TFTP server files
    #    ${LWIPDIR}/apps/tftp/tftp_server.c
    #
    #
    #    # MQTTFILES: MQTT client files
    #    ${LWIPDIR}/apps/mqtt/mqtt.c


    # patch files
    ${LWIPARCH}/perf.c
    ${LWIPARCH}/sys_arch.c
    )
