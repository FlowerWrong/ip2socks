cmake_minimum_required(VERSION 2.8)
project(ip2socks)

set(CMAKE_CXX_STANDARD 11)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -g -Wall -DLWIP_DEBUG")

set(LIBEVDIR libev)
set(LIBSDS sds)
set(LWIPDIR lwip/src)
set(LWIPARCH port)

include(cmake/libev.cmake)
include(cmake/lwip.cmake)

include_directories(
    ${LWIPDIR}/include
    ${LWIPARCH}/include

    ${LIBEVDIR}
    
    src
)


set(MAIN_SOURCE_FILES
    ${LWIP_SOURCE_FILES}

    # patch files
    ${LWIPARCH}/netif/tunif.c
    ${LWIPARCH}/netif/socket_util.c

    ${LIBEVDIR}/ev.c

    src/socks5.cpp
    src/tcp_raw.cpp
    src/udp_raw.cpp
    src/main.cpp
    )

if (UNIX AND NOT APPLE)
    set(MAIN_SOURCE_FILES

        ${MAIN_SOURCE_FILES}

        # patch files
        ${LWIPARCH}/netif/tapif.c
        )
endif ()

add_executable(ip2socks ${MAIN_SOURCE_FILES})
