cmake_minimum_required(VERSION 2.8)
project(ip2socks)

set(CMAKE_CXX_STANDARD 11)

set(LIBEVDIR libev)
set(LIBYAML libyaml)
set(LWIPDIR lwip/src)
set(LWIPARCH port)

set(LIB_RUBY_DIR ruby)

include(cmake/liblwip.cmake)
include(cmake/libyaml.cmake)
include(cmake/libev.cmake)

include_directories(
    ${LWIPDIR}/include
    ${LWIPARCH}/include

    ${LIBEVDIR}

    ${PROJECT_SOURCE_DIR}/build/ruby/include/ruby-2.5.0
    ${PROJECT_SOURCE_DIR}/build/ruby/include/ruby-2.5.0/x86_64-darwin16

    ${LIBYAML}/include
    ${LIBYAML}/win32
    ${LIBYAML}/src

    src
    src/netif
    src/dns
)


set(MAIN_SOURCE_FILES
    ${LWIP_SOURCE_FILES}

    ${LIBEVDIR}/ev.c

    # patch files
    src/netif/tunif.c
    src/netif/socket_util.c

    src/dns/dump_dns.c

    src/struct.cpp
    src/socks5.cpp
    src/util.cpp
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
target_link_libraries(ip2socks resolv)

target_link_libraries(ip2socks ${PROJECT_SOURCE_DIR}/build/ruby/lib/libruby.2.5.0.dylib)
