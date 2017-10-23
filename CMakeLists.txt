cmake_minimum_required(VERSION 2.8)
project(ip2socks)

set(CMAKE_CXX_STANDARD 11)

set(LIBEVDIR libev)
set(LIBYAML libyaml)
set(LWIPDIR lwip/src)
set(LWIPARCH port)

set(LIB_RUBY_DIR ${PROJECT_SOURCE_DIR}/build/ruby)

include(cmake/liblwip.cmake)
include(cmake/libyaml.cmake)
include(cmake/libev.cmake)

include_directories(
    # lwip and patch
    ${LWIPDIR}/include
    ${LWIPARCH}/include

    # libev
    ${LIBEVDIR}

    # ruby
    ${LIB_RUBY_DIR}/include/ruby-2.4.0

    # libyaml
    ${LIBYAML}/include
    ${LIBYAML}/win32
    ${LIBYAML}/src

    # source code
    src/netif
    src/dns
    src/ruby
    src
)

# ruby config.h
if (UNIX AND NOT APPLE)
    include_directories(${LIB_RUBY_DIR}/include/ruby-2.4.0/x86_64-linux)
endif ()
if (UNIX AND APPLE)
    include_directories(${LIB_RUBY_DIR}/include/ruby-2.4.0/x86_64-darwin16)
    include_directories(${LIB_RUBY_DIR}/include/ruby-2.4.0/x86_64-darwin17) # FIXME
endif ()


set(MAIN_SOURCE_FILES
    # lwip and patch
    ${LWIP_SOURCE_FILES}

    # libev
    ${LIBEVDIR}/ev.c

    # patch files
    src/netif/tunif.c
    src/netif/socket_util.c

    src/dns/dump_dns.c

    src/ruby/rb_ev_io.c

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

if (UNIX AND NOT APPLE)
    target_link_libraries(ip2socks ${LIB_RUBY_DIR}/lib/libruby.so)
endif ()
if (UNIX AND APPLE)
    target_link_libraries(ip2socks ${LIB_RUBY_DIR}/lib/libruby.dylib)
endif ()
