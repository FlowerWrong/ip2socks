PROJECT(librdns C)

SET(RDNS_VERSION_MAJOR 0)
SET(RDNS_VERSION_MINOR 1)
SET(RDNS_VERSION_PATCH 1)

OPTION(ENABLE_CURVE "Enable DNSCurve plugin" ON)

INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)

SET(LIBRDNSSRC
    ${CMAKE_CURRENT_SOURCE_DIR}/librdns/src/util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/librdns/src/logger.c
    ${CMAKE_CURRENT_SOURCE_DIR}/librdns/src/compression.c
    ${CMAKE_CURRENT_SOURCE_DIR}/librdns/src/punycode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/librdns/src/curve.c
    ${CMAKE_CURRENT_SOURCE_DIR}/librdns/src/parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/librdns/src/packet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/librdns/src/resolver.c)

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/librdns/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/librdns/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/librdns/contrib/uthash"
    "${CMAKE_CURRENT_SOURCE_DIR}/librdns/contrib/libottery"
    "${CMAKE_CURRENT_SOURCE_DIR}/librdns/contrib/tweetnacl")

IF (NOT SLAVE_BUILD)
    CHECK_SYMBOL_EXISTS(IPV6_V6ONLY "sys/socket.h;netinet/in.h" HAVE_IPV6_V6ONLY)

    CHECK_SYMBOL_EXISTS(getaddrinfo "sys/types.h;sys/socket.h;netdb.h" HAVE_GETADDRINFO)
    IF (NOT HAVE_GETADDRINFO)
        MESSAGE(FATAL_ERROR "Your system does not support getaddrinfo call, please consider upgrading it to run librdns")
    ENDIF (NOT HAVE_GETADDRINFO)

    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

    IF (ENABLE_CURVE MATCHES "ON")
        ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/librdns/contrib/tweetnacl)
        INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/librdns/contrib/tweetnacl")
        SET(TWEETNACL 1)
        ADD_DEFINITIONS("-DTWEETNACL")
    ENDIF (ENABLE_CURVE MATCHES "ON")

    ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/librdns/contrib/libottery)
    ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/librdns/test)

    ADD_LIBRARY(rdns_core OBJECT ${LIBRDNSSRC})
    SET_TARGET_PROPERTIES(rdns_core PROPERTIES COMPILE_FLAGS "-fPIC")

    SET(DLIBS "$<TARGET_OBJECTS:ottery>")
    IF (ENABLE_CURVE MATCHES "ON")
        LIST(APPEND DLIBS "$<TARGET_OBJECTS:tweetnacl>")
    ENDIF (ENABLE_CURVE MATCHES "ON")

    ADD_LIBRARY(rdns_static STATIC $<TARGET_OBJECTS:rdns_core> ${DLIBS})
    ADD_LIBRARY(rdns SHARED $<TARGET_OBJECTS:rdns_core> ${DLIBS})
    SET_TARGET_PROPERTIES(rdns PROPERTIES
        VERSION ${RDNS_VERSION_MAJOR}.${RDNS_VERSION_MINOR}.${RDNS_VERSION_PATCH}
        SOVERSION ${RDNS_VERSION_MINOR})
ELSE (NOT SLAVE_BUILD)
    ADD_LIBRARY(rdns STATIC ${LIBRDNSSRC})
ENDIF (NOT SLAVE_BUILD)
