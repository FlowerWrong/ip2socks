cmake_minimum_required(VERSION 2.8)
project(lua C)

set(LIBLUADIR lua)

include_directories(${LIBLUADIR}/src ${CMAKE_CURRENT_BINARY_DIR})
set(SRC_CORE
    ${LIBLUADIR}/src/lapi.c
    ${LIBLUADIR}/src/lcode.c
    ${LIBLUADIR}/src/lctype.c
    ${LIBLUADIR}/src/ldebug.c
    ${LIBLUADIR}/src/ldo.c
    ${LIBLUADIR}/src/ldump.c
    ${LIBLUADIR}/src/lfunc.c
    ${LIBLUADIR}/src/lgc.c
    ${LIBLUADIR}/src/llex.c
    ${LIBLUADIR}/src/lmem.c
    ${LIBLUADIR}/src/lobject.c
    ${LIBLUADIR}/src/lopcodes.c
    ${LIBLUADIR}/src/lparser.c
    ${LIBLUADIR}/src/lstate.c
    ${LIBLUADIR}/src/lstring.c
    ${LIBLUADIR}/src/ltable.c
    ${LIBLUADIR}/src/ltm.c
    ${LIBLUADIR}/src/lundump.c
    ${LIBLUADIR}/src/lvm.c
    ${LIBLUADIR}/src/lzio.c)
set(SRC_LIB
    ${LIBLUADIR}/src/lauxlib.c
    ${LIBLUADIR}/src/lbaselib.c
    ${LIBLUADIR}/src/lbitlib.c
    ${LIBLUADIR}/src/lcorolib.c
    ${LIBLUADIR}/src/ldblib.c
    ${LIBLUADIR}/src/liolib.c
    ${LIBLUADIR}/src/lmathlib.c
    ${LIBLUADIR}/src/loslib.c
    ${LIBLUADIR}/src/lstrlib.c
    ${LIBLUADIR}/src/ltablib.c
    ${LIBLUADIR}/src/lutf8lib.c
    ${LIBLUADIR}/src/loadlib.c
    ${LIBLUADIR}/src/linit.c)

set(SRC_LUA ${LIBLUADIR}/src/lua.c)
set(SRC_LUAC ${LIBLUADIR}/src/luac.c)

add_library(liblua STATIC ${SRC_CORE} ${SRC_LIB})
set_target_properties(liblua PROPERTIES OUTPUT_NAME lua)
