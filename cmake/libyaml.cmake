cmake_minimum_required(VERSION 2.8)
project(yaml C)

set(YAML_VERSION_MAJOR 0)
set(YAML_VERSION_MINOR 1)
set(YAML_VERSION_PATCH 7)
set(YAML_VERSION_STRING "${YAML_VERSION_MAJOR}.${YAML_VERSION_MINOR}.${YAML_VERSION_PATCH}")

set(LIBYAMLDIR libyaml)

add_definitions(-DHAVE_CONFIG_H)

file(GLOB YAML_SOURCE_FILES ${LIBYAMLDIR}/src/*.c ${LIBYAMLDIR}/src/yaml_private.h)

configure_file(cmake/config.h.cmake ${LIBYAMLDIR}/include/config.h)
