cmake_minimum_required(VERSION 2.8)
project(ip2socks)

set(CMAKE_CXX_STANDARD 11)

set(LIBEVDIR libev)
set(LIBYAML libyaml)
set(LWIPDIR lwip/src)
set(LWIPARCH port)
set(LUADIR lua)
set(LIB_DUKTAPE_DIR duktape)
set(LIB_MRUBY_DIR mruby)

include(cmake/liblwip.cmake)
include(cmake/libyaml.cmake)
include(cmake/libev.cmake)
include(cmake/liblua.cmake)
include(cmake/libduktape.cmake)

include_directories(
    ${LWIPDIR}/include
    ${LWIPARCH}/include

    ${LIBEVDIR}

    ${LIB_DUKTAPE_DIR}/dist/src

    ${LIB_MRUBY_DIR}/include

    ${LIBYAML}/include
    ${LIBYAML}/win32
    ${LIBYAML}/src

    ${LUADIR}/src

    src
    src/netif
    src/dns
    src/duktape_socket
    src/mruby_ev
)


set(MAIN_SOURCE_FILES
    ${LWIP_SOURCE_FILES}

    # patch files
    src/netif/tunif.c
    src/netif/socket_util.c

    ${LIBEVDIR}/ev.c

    ${DUKTAPE_SOURCE_FILES}

    src/dns/dump_dns.c

    src/duktape_socket/socket.c

    src/mruby_ev/mrev.c

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
target_link_libraries(ip2socks lua)
target_link_libraries(ip2socks resolv)

target_link_libraries(ip2socks ${CMAKE_SOURCE_DIR}/${LIB_MRUBY_DIR}/build/host/lib/libmruby.a)
