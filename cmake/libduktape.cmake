cmake_minimum_required(VERSION 2.8)
project(duktape C)

set(LIB_DUKTAPE_DIR duktape)

# SET(PYTHON_EXECUTABLE "python2")
# execute_process(COMMAND ${PYTHON_EXECUTABLE} "${LIB_DUKTAPE_DIR}/util/dist.py")

include_directories(${LIB_DUKTAPE_DIR}/dist/src)

set(DUKTAPE_SOURCE_FILES
    ${LIB_DUKTAPE_DIR}/dist/src/duktape.c
    )
