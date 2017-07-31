cmake_minimum_required(VERSION 2.8)
project(yaml C)

set(YAML_VERSION_MAJOR 0)
set(YAML_VERSION_MINOR 1)
set(YAML_VERSION_PATCH 7)
set(YAML_VERSION_STRING "${YAML_VERSION_MAJOR}.${YAML_VERSION_MINOR}.${YAML_VERSION_PATCH}")

set(LIBYAMLDIR libyaml)

file(GLOB SRC ${LIBYAMLDIR}/src/*.c)

# configure_file(cmake/yaml_config.h.cmake ${LIBYAMLDIR}/include/config.h)

set(config_h ${LIBYAMLDIR}/include/config.h)
configure_file(
    cmake/yaml_config.h.cmake
    ${config_h}
)

include_directories(${LIBYAMLDIR}/include ${LIBYAMLDIR}/win32)

add_library(yaml_static STATIC ${SRC})
set_target_properties(yaml_static PROPERTIES COMPILE_FLAGS "-DYAML_DECLARE_STATIC -DHAVE_CONFIG_H")

if (UNIX AND NOT APPLE)
    add_library(yaml SHARED ${SRC})
    set_target_properties(yaml PROPERTIES COMPILE_FLAGS "-DYAML_DECLARE_EXPORT -DHAVE_CONFIG_H")
endif ()
