cmake_minimum_required(VERSION 2.8)

project(lua C)

set(LIBLUADIR lua)

set(CMAKE_MACOSX_RPATH 1)

add_definitions(-DLUA_USE_POSIX -DLUA_USE_DLOPEN)
set(LIBS m dl)


# Add Readline support when available
find_path(READLINE_INCLUDE_DIR readline/readline.h)
find_library(READLINE_LIBRARY NAMES readline)
IF (READLINE_LIBRARY)
    include_directories(${READLINE_INCLUDE_DIR})
    add_definitions(-DLUA_USE_READLINE)
    set(LIBS ${LIBS} ${READLINE_LIBRARY})
ENDIF ()

# Add Curses support when available
include(FindCurses)
IF (CURSES_LIBRARY)
    include_directories(${CURSES_INCLUDE_DIR})
    set(LIBS ${LIBS} ${CURSES_LIBRARY})
ENDIF ()

# Build Libraries
set(SRC_LIBLUA
    ${LIBLUADIR}/src/lapi.c
    ${LIBLUADIR}/src/lauxlib.c
    ${LIBLUADIR}/src/lbaselib.c
    ${LIBLUADIR}/src/lcode.c
    ${LIBLUADIR}/src/lcorolib.c
    ${LIBLUADIR}/src/lctype.c
    ${LIBLUADIR}/src/ldblib.c
    ${LIBLUADIR}/src/ldebug.c
    ${LIBLUADIR}/src/ldo.c
    ${LIBLUADIR}/src/ldump.c
    ${LIBLUADIR}/src/lfunc.c
    ${LIBLUADIR}/src/lgc.c
    ${LIBLUADIR}/src/linit.c
    ${LIBLUADIR}/src/liolib.c
    ${LIBLUADIR}/src/llex.c
    ${LIBLUADIR}/src/lmathlib.c
    ${LIBLUADIR}/src/lmem.c
    ${LIBLUADIR}/src/loadlib.c
    ${LIBLUADIR}/src/lobject.c
    ${LIBLUADIR}/src/lopcodes.c
    ${LIBLUADIR}/src/loslib.c
    ${LIBLUADIR}/src/lparser.c
    ${LIBLUADIR}/src/lstate.c
    ${LIBLUADIR}/src/lstring.c
    ${LIBLUADIR}/src/lstrlib.c
    ${LIBLUADIR}/src/ltable.c
    ${LIBLUADIR}/src/ltablib.c
    ${LIBLUADIR}/src/ltm.c
    ${LIBLUADIR}/src/lundump.c
    ${LIBLUADIR}/src/lutf8lib.c
    ${LIBLUADIR}/src/lvm.c
    ${LIBLUADIR}/src/lzio.c
    )

include_directories(${LUADIR/src})

add_library(lua SHARED ${SRC_LIBLUA})
target_link_libraries(lua ${LIBS})

add_library(lua_static ${SRC_LIBLUA})
target_link_libraries(lua_static ${LIBS})
