cmake_minimum_required(VERSION 2.8)
project(ip2socks C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -g -Wall -DLWIP_DEBUG")

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
    ${LIBSDS}
    
    src
)


set(MAIN_SOURCE_FILES
    
    sds/sds.c

    ${LWIP_SOURCE_FILES}

    # patch files
    ${LWIPARCH}/netif/tunif.c

    ${LIBEVDIR}/ev.c

    src/socks5.c
    src/tcpecho_raw.c
    src/udpecho_raw.c
    src/main.c
    )

if (UNIX AND NOT APPLE)
    set(MAIN_SOURCE_FILES

        ${MAIN_SOURCE_FILES}

        # patch files
        ${LWIPARCH}/netif/tapif.c
        )
endif ()

add_executable(ip2socks ${MAIN_SOURCE_FILES})
