cmake_minimum_required(VERSION 2.8)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_module")

project(ip2socks)

set(CMAKE_CXX_STANDARD 11)

set(LIBEVDIR libev)
set(LIBYAML libyaml)
set(LWIPDIR lwip/src)
set(LWIPARCH lwip-contrib/ports/unix/port)

include(cmake/liblwip.cmake)
include(cmake/libyaml.cmake)
include(cmake/libev.cmake)

include_directories(
    # lwip and patch
    ${LWIPDIR}/include
    ${LWIPARCH}/include

    # libev
    ${LIBEVDIR}

    # libyaml
    ${LIBYAML}/include
    ${LIBYAML}/win32
    ${LIBYAML}/src

    # source code
    src/netif
    src/dns
    src
)

set(MAIN_SOURCE_FILES
    # lwip and patch
    ${LWIP_SOURCE_FILES}

    # libev
    ${LIBEVDIR}/ev.c

    # patch files
    src/netif/tunif.c
    src/netif/socket_util.c

    src/dns/dns_parser.c

    src/struct.cpp
    src/socks5.cpp
    src/util.cpp
    src/tcp_raw.cpp
    src/udp_raw.cpp
    src/main.cpp
    )

# linux
if (UNIX AND NOT APPLE)
    set(MAIN_SOURCE_FILES
        ${MAIN_SOURCE_FILES}
        # patch files
        src/netif/tapif.c
        )
endif ()

add_executable(ip2socks ${MAIN_SOURCE_FILES})

target_link_libraries(ip2socks yaml)
target_link_libraries(ip2socks resolv)
