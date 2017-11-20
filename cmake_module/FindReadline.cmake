# - Try to find readline include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(Readline)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  READLINE_ROOT_DIR         Set this variable to the root installation of
#                            readline if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  READLINE_FOUND            System has readline, include and lib dirs found
#  READLINE_INCLUDE_DIR      The readline include directories.
#  READLINE_LIBRARIES          The readline library.

include(FindPackageHandleStandardArgs)

find_path(READLINE_ROOT_DIR
  NAMES include/readline/readline.h
)

find_path(READLINE_INCLUDE_DIR
  NAMES readline/readline.h
  HINTS ${READLINE_ROOT_DIR}/include
)

find_library(READLINE_LIBRARY
  NAMES readline
  HINTS ${READLINE_ROOT_DIR}/lib
)

#find_package(Curses)
find_package_handle_standard_args(Readline
  DEFAULT_MSG
#    CURSES_FOUND
    READLINE_INCLUDE_DIR
    READLINE_LIBRARY
)

set(READLINE_LIBRARIES ${READLINE_LIBRARY} ${CURSES_LIBRARIES})
set(READLINE_INCLUDE_DIR ${READLINE_INCLUDE_DIR} ${CURSES_INCLUDE_DIR})

mark_as_advanced(READLINE_ROOT_DIR READLINE_INCLUDE_DIR READLINE_LIBRARY)
