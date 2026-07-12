cmake_minimum_required(VERSION 3.15)

# Derive version from the header so CMake, Makefile, and pkg-config
# stay in lockstep — one source of truth.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/ctestprobe.h" _hdr)
string(REGEX MATCH "CTESTPROBE_VERSION_MAJOR ([0-9]+)" _m "${_hdr}")
set(_major ${CMAKE_MATCH_1})
string(REGEX MATCH "CTESTPROBE_VERSION_MINOR ([0-9]+)" _m "${_hdr}")
set(_minor ${CMAKE_MATCH_1})
string(REGEX MATCH "CTESTPROBE_VERSION_PATCH ([0-9]+)" _m "${_hdr}")
set(_patch ${CMAKE_MATCH_1})

project(ctestprobe
    VERSION ${_major}.${_minor}.${_patch}
    DESCRIPTION "Minimal unit-testing framework for C"
    HOMEPAGE_URL "https://github.com/ocrosby/ctestprobe"
    LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(ctestprobe STATIC src/ctestprobe.c)
add_library(ctestprobe::ctestprobe ALIAS ctestprobe)

target_include_directories(ctestprobe
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_options(ctestprobe PRIVATE
    -Wall -Wextra -Wpedantic)

option(CTESTPROBE_BUILD_TESTS    "Build the self-test binary" ON)
option(CTESTPROBE_BUILD_EXAMPLES "Build the example binary"   ON)

if(CTESTPROBE_BUILD_TESTS)
    enable_testing()
    add_executable(test_ctestprobe tests/test_ctestprobe.c)
    target_link_libraries(test_ctestprobe PRIVATE ctestprobe::ctestprobe)
    add_test(NAME self_test COMMAND test_ctestprobe)
endif()

if(CTESTPROBE_BUILD_EXAMPLES)
    add_executable(example1 examples/example1.c)
    target_link_libraries(example1 PRIVATE ctestprobe::ctestprobe)
endif()

# ---- Install ----
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ctestprobe
    EXPORT ctestprobe-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES include/ctestprobe.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ctestprobe-targets
    FILE ctestprobe-targets.cmake
    NAMESPACE ctestprobe::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ctestprobe)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ctestprobe-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ctestprobe-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ctestprobe)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ctestprobe-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ctestprobe-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ctestprobe-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ctestprobe)

# pkg-config for CMake consumers that use it
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ctestprobe.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ctestprobe.pc"
    @ONLY)
# Manually substitute the placeholders that use @-syntax vs the .in file's own conventions
file(READ "${CMAKE_CURRENT_BINARY_DIR}/ctestprobe.pc" _pc)
string(REPLACE "@PREFIX@"  "${CMAKE_INSTALL_PREFIX}" _pc "${_pc}")
string(REPLACE "@VERSION@" "${PROJECT_VERSION}"     _pc "${_pc}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ctestprobe.pc" "${_pc}")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ctestprobe.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
