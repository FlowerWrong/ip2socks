cmake_minimum_required(VERSION 2.8)

project(ev C)

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)

set(LIBEVDIR libev)

include_directories(${LIBEVDIR})

check_include_files(sys/inotify.h HAVE_SYS_INOTIFY_H)
check_include_files(sys/epoll.h HAVE_SYS_EPOLL_H)
check_include_files(sys/event.h HAVE_SYS_EVENT_H)
check_include_files(sys/queue.h HAVE_SYS_QUEUE_H)
check_include_files(port.h HAVE_PORT_H)
check_include_files(poll.h HAVE_POLL_H)
check_include_files(sys/select.h HAVE_SYS_SELECT_H)
check_include_files(sys/eventfd.h HAVE_SYS_EVENTFD_H)

check_function_exists(inotify_init HAVE_INOTIFY_INIT)
check_function_exists(epoll_ctl HAVE_EPOLL_CTL)
check_function_exists(kqueue HAVE_KQUEUE)
check_function_exists(port_create HAVE_PORT_CREATE)
check_function_exists(poll HAVE_POLL)
check_function_exists(select HAVE_SELECT)
check_function_exists(eventfd HAVE_EVENTFD)
check_function_exists(nanosleep HAVE_NANOSLEEP)


configure_file(${LIBEVDIR}/config.h.cmake ${LIBEVDIR}/config.h)
