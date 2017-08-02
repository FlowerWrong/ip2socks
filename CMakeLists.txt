cmake_minimum_required(VERSION 2.8)
project(ip2socks)

set(CMAKE_CXX_STANDARD 11)

set(LIBEVDIR libev)
set(LIBYAML libyaml)
set(LIBRDNS librdns)
set(LWIPDIR lwip/src)
set(LWIPARCH port)

include(cmake/liblwip.cmake)
include(cmake/libyaml.cmake)
include(cmake/libev.cmake)
include(cmake/librdns.cmake)

include_directories(
    ${LWIPDIR}/include
    ${LWIPARCH}/include

    ${LIBEVDIR}

    ${LIBRDNS}/src
    ${LIBRDNS}/include
    ${LIBRDNS}/contrib/uthash
    ${LIBRDNS}/contrib/libottery
    ${LIBRDNS}/contrib/tweetnacl

    ${LIBYAML}/include
    ${LIBYAML}/win32
    ${LIBYAML}/src

    src
    src/netif
    src/dns
)


set(MAIN_SOURCE_FILES
    ${LWIP_SOURCE_FILES}

    # patch files
    src/netif/tunif.c
    src/netif/socket_util.c

    ${LIBEVDIR}/ev.c

    src/dns/dump_dns.c
    
    src/struct.cpp
    src/socks5.cpp
    src/tcp_raw.cpp
    src/udp_raw.cpp
    src/main.cpp
    )

if (UNIX AND NOT APPLE)
    set(MAIN_SOURCE_FILES
        ${MAIN_SOURCE_FILES}
        # patch files
        src/netif/tapif.c
        )
endif ()

add_executable(ip2socks ${MAIN_SOURCE_FILES})

target_link_libraries(ip2socks yaml)
target_link_libraries(ip2socks rdns)
target_link_libraries(ip2socks resolv)
