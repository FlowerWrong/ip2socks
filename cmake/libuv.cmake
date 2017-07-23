cmake_minimum_required(VERSION 2.8)
project(uv C)

# start build libuv
set(LIBUVDIR libuv)
include_directories(${LIBUVDIR}/include ${LIBUVDIR}/src)

set(SOURCES
    ${LIBUVDIR}/include/uv.h
    ${LIBUVDIR}/include/tree.h
    ${LIBUVDIR}/include/uv-errno.h
    ${LIBUVDIR}/include/uv-threadpool.h
    ${LIBUVDIR}/include/uv-version.h
    ${LIBUVDIR}/src/fs-poll.c
    ${LIBUVDIR}/src/heap-inl.h
    ${LIBUVDIR}/src/inet.c
    ${LIBUVDIR}/src/queue.h
    ${LIBUVDIR}/src/threadpool.c
    ${LIBUVDIR}/src/uv-common.c
    ${LIBUVDIR}/src/uv-common.h
    ${LIBUVDIR}/src/version.c
    )

if (WIN32)
    add_definitions(-D_WIN32_WINNT=0x0600 -D_GNU_SOURCE -D_CRT_SECURE_NO_WARNINGS)
    set(SOURCES ${SOURCES}
        ${LIBUVDIR}/include/uv-win.h
        ${LIBUVDIR}/src/win/async.c
        ${LIBUVDIR}/src/win/atomicops-inl.h
        ${LIBUVDIR}/src/win/core.c
        ${LIBUVDIR}/src/win/detect-wakeup.c
        ${LIBUVDIR}/src/win/dl.c
        ${LIBUVDIR}/src/win/error.c
        ${LIBUVDIR}/src/win/fs.c
        ${LIBUVDIR}/src/win/fs-event.c
        ${LIBUVDIR}/src/win/getaddrinfo.c
        ${LIBUVDIR}/src/win/getnameinfo.c
        ${LIBUVDIR}/src/win/handle.c
        ${LIBUVDIR}/src/win/handle-inl.h
        ${LIBUVDIR}/src/win/internal.h
        ${LIBUVDIR}/src/win/loop-watcher.c
        ${LIBUVDIR}/src/win/pipe.c
        ${LIBUVDIR}/src/win/thread.c
        ${LIBUVDIR}/src/win/poll.c
        ${LIBUVDIR}/src/win/process.c
        ${LIBUVDIR}/src/win/process-stdio.c
        ${LIBUVDIR}/src/win/req.c
        ${LIBUVDIR}/src/win/req-inl.h
        ${LIBUVDIR}/src/win/signal.c
        ${LIBUVDIR}/src/win/stream.c
        ${LIBUVDIR}/src/win/stream-inl.h
        ${LIBUVDIR}/src/win/tcp.c
        ${LIBUVDIR}/src/win/tty.c
        ${LIBUVDIR}/src/win/timer.c
        ${LIBUVDIR}/src/win/udp.c
        ${LIBUVDIR}/src/win/util.c
        ${LIBUVDIR}/src/win/winapi.c
        ${LIBUVDIR}/src/win/winapi.h
        ${LIBUVDIR}/src/win/winsock.c
        ${LIBUVDIR}/src/win/winsock.h
        )

    add_library(uv ${SOURCES})
    target_link_libraries(uv advapi32 iphlpapi psapi userenv shell32 ws2_32)
else ()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g --std=gnu89 -pedantic -Wall -Wextra -Wno-unused-parameter")
    add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64)
    include_directories(${LIBUVDIR}/src/unix)
    set(SOURCES ${SOURCES}
        ${LIBUVDIR}/include/uv-unix.h
        ${LIBUVDIR}/include/uv-linux.h
        ${LIBUVDIR}/include/uv-sunos.h
        ${LIBUVDIR}/include/uv-darwin.h
        ${LIBUVDIR}/include/uv-bsd.h
        ${LIBUVDIR}/include/uv-aix.h
        ${LIBUVDIR}/src/unix/async.c
        ${LIBUVDIR}/src/unix/atomic-ops.h
        ${LIBUVDIR}/src/unix/core.c
        ${LIBUVDIR}/src/unix/dl.c
        ${LIBUVDIR}/src/unix/fs.c
        ${LIBUVDIR}/src/unix/getaddrinfo.c
        ${LIBUVDIR}/src/unix/getnameinfo.c
        ${LIBUVDIR}/src/unix/internal.h
        ${LIBUVDIR}/src/unix/loop.c
        ${LIBUVDIR}/src/unix/loop-watcher.c
        ${LIBUVDIR}/src/unix/pipe.c
        ${LIBUVDIR}/src/unix/poll.c
        ${LIBUVDIR}/src/unix/process.c
        ${LIBUVDIR}/src/unix/signal.c
        ${LIBUVDIR}/src/unix/spinlock.h
        ${LIBUVDIR}/src/unix/stream.c
        ${LIBUVDIR}/src/unix/tcp.c
        ${LIBUVDIR}/src/unix/thread.c
        ${LIBUVDIR}/src/unix/timer.c
        ${LIBUVDIR}/src/unix/tty.c
        ${LIBUVDIR}/src/unix/udp.c
        )

    if (APPLE)
        add_definitions(-D_DARWIN_USE_64_BIT_INODE=1 -D_DARWIN_UNLIMITED_SELECT=1)
        set(SOURCES ${SOURCES}
            ${LIBUVDIR}/src/unix/proctitle.c
            ${LIBUVDIR}/src/unix/kqueue.c
            ${LIBUVDIR}/src/unix/darwin.c
            ${LIBUVDIR}/src/unix/fsevents.c
            ${LIBUVDIR}/src/unix/pthread-barrier.c
            ${LIBUVDIR}/src/unix/darwin-proctitle.c
            )

        #This is necessary to mute harmless warnings, as documented by https://github.com/libuv/libuv/issues/454
        set_source_files_properties(${LIBUVDIR}/src/unix/stream.c PROPERTIES COMPILE_FLAGS -Wno-gnu-folding-constant)
    endif ()

    if (NOT APPLE)
        add_definitions(-Wstrict-aliasing)
    endif ()

    if ((${CMAKE_SYSTEM_NAME} MATCHES "Linux") OR (${CMAKE_SYSTEM_NAME} MATCHES "Android"))
        add_definitions(-D_GNU_SOURCE -D_POSIX_C_SOURCE=200112)
        set(SOURCES ${SOURCES}
            ${LIBUVDIR}/src/unix/proctitle.c
            ${LIBUVDIR}/src/unix/linux-core.c
            ${LIBUVDIR}/src/unix/linux-inotify.c
            ${LIBUVDIR}/src/unix/linux-syscalls.c
            ${LIBUVDIR}/src/unix/linux-syscalls.h
            )
    endif ()

    if (${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
        add_definitions(-D__EXTENSIONS__ -D_XOPEN_SOURCE=500)
        set(SOURCES ${SOURCES}
            ${LIBUVDIR}/src/unix/sunos.c
            )
    endif ()

    if (${CMAKE_SYSTEM_NAME} MATCHES "AIX")
        add_definitions(-D_ALL_SOURCE -D_XOPEN_SOURCE=500 -D_LINUX_SOURCE_COMPAT)
        set(SOURCES ${SOURCES}
            ${LIBUVDIR}/src/unix/aic.c
            )
        find_library(PERFSTAT_LIB NAMES perfstat)
    endif ()

    if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "DragonFlyBSD")
        set(SOURCES ${SOURCES}
            ${LIBUVDIR}/src/unix/freebsd.c
            )
    endif ()

    if (${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
        set(SOURCES ${SOURCES}
            ${LIBUVDIR}/src/unix/openbsd.c
            )
    endif ()

    if (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
        set(SOURCES ${SOURCES}
            ${LIBUVDIR}/src/unix/netbsd.c
            )
    endif ()

    if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "DragonFlyBSD" OR
        ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
        set(SOURCES ${SOURCES}
            ${LIBUVDIR}/src/unix/kqueue.c
            )
    endif ()

    if (${CMAKE_SYSTEM_NAME} MATCHES "Android")
        set(SOURCES ${SOURCES}
            ${LIBUVDIR}/include/android-ifaddrs.h
            ${LIBUVDIR}/include/pthread-fixes.h
            #${LIBUVDIR}/include/pthread-barrier.h   ## since 1.10 `fixes` renamed to `barrier`
            ${LIBUVDIR}/src/unix/android-ifaddrs.c
            ${LIBUVDIR}/src/unix/pthread-fixes.c
            #${LIBUVDIR}/src/unix/pthread-barrier.c  ## since 1.10 `fixes` renamed to `barrier`
            )
    endif ()

    add_library(uv ${SOURCES})

    find_library(M_LIB NAMES m)
    find_package(Threads)
    target_link_libraries(uv ${CMAKE_THREAD_LIBS_INIT} ${M_LIB})

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        find_library(DL_LIB NAMES dl)
        find_library(RT_LIB NAMES rt)
        target_link_libraries(uv ${DL_LIB} ${RT_LIB})
    endif ()

    if (${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
        find_library(KSTAT_LIB NAMES kstat)
        find_library(NSL_LIB NAMES nsl)
        find_library(SENDFILE_LIB NAMES sendfile)
        find_library(SOCKET_LIB NAMES socket)
        target_link_libraries(uv ${KSTAT_LIB} ${NSL_LIB} ${SENDFILE_LIB} ${SOCKET_LIB})
    endif ()

    if (${CMAKE_SYSTEM_NAME} MATCHES "AIX")
        find_library(PERFSTAT_LIB NAMES perfstat)
        target_link_libraries(uv ${PERFSTAT_LIB})
    endif ()

    if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "DragonFlyBSD" OR
        ${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
        find_library(KVM_LIB NAMES kvm)
        target_link_libraries(uv ${KVM_LIB})
    endif ()

endif ()
# end build libuv
