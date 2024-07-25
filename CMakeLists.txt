#
# Copyright (c) 2022 ZettaScale Technology
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
# ZettaScale Zenoh Team, <zenoh@zettascale.tech>
#
cmake_minimum_required(VERSION 3.13)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/version.txt version)

project(zenohpico VERSION ${version} LANGUAGES C)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(helpers)

# Configure header file to define the project version
set(ZENOH_PICO ${PROJECT_VERSION})
set(ZENOH_PICO_MAJOR ${PROJECT_VERSION_MAJOR})
set(ZENOH_PICO_MINOR ${PROJECT_VERSION_MINOR})
set(ZENOH_PICO_PATCH ${PROJECT_VERSION_PATCH})
set(ZENOH_PICO_TWEAK ${PROJECT_VERSION_TWEAK})

set(project_version "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
if(NOT DEFINED PROJECT_VERSION_TWEAK)
	set(project_version "${project_version}")
elseif(PROJECT_VERSION_TWEAK EQUAL 0)
	set(project_version "${project_version}-dev")
elseif(PROJECT_VERSION_TWEAK GREATER 1)
	set(project_version "${project_version}-pre.${PROJECT_VERSION_TWEAK}")
endif()
status_print(project_version)


include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build shared libraries if ON, otherwise build static libraries" ON)
option(WITH_ZEPHYR "Build for Zephyr RTOS" OFF)
option(WITH_FREERTOS_PLUS_TCP "Build for FreeRTOS RTOS and FreeRTOS-Plus-TCP network stack" OFF)
set(ZENOH_DEBUG 0 CACHE STRING "Use this to set the ZENOH_DEBUG variable")
set(FRAG_MAX_SIZE 0 CACHE STRING "Use this to override the maximum size for fragmented messages")
set(BATCH_UNICAST_SIZE 0 CACHE STRING "Use this to override the maximum unicast batch size")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(BUILD_SHARED_LIBS "OFF")
elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
  if(WITH_ZEPHYR)
    set(BUILD_SHARED_LIBS "OFF")
  endif()
endif()

# Language options
if(NOT CMAKE_C_STANDARD)
  if(c_std_11 IN_LIST CMAKE_C_COMPILE_FEATURES)
    set(CMAKE_C_STANDARD 11)
    message(STATUS "Setting C11 as the C Standard")
  else()
    # C99 pedantic doesn't like unix header anonymous structure
    set(CMAKE_C_STANDARD 99)
    message(STATUS "Setting C99 as the C Standard")
  endif()
endif()
set(CMAKE_C_STANDARD_REQUIRED TRUE)

# Compile options
if(CMAKE_BUILD_TYPE MATCHES "RELEASE" OR "Release")
  if(UNIX)
    add_compile_options(-pipe -O3)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
    add_compile_options(-pipe -O3)
  endif()
else()
  if(UNIX)
    add_compile_options(-c -Wall -Wextra -Werror -Wshadow -Wunused -Wstrict-prototypes -pipe -g -O0)
    # C99 pedantic doesn't like struct anonymous in unix header
    if (NOT CMAKE_C_STANDARD STREQUAL "99")
      add_compile_options(-Wpedantic)
    endif()
    # add_compile_options(-Wconversion)
    # add_link_options(-fsanitize=address)
  elseif(MSVC)
    add_compile_options(/W4 /WX /Od /wd4127)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wmissing-prototypes -pipe -g -O0)
  endif()
endif()

set(Libname "zenohpico")
if(BUILD_SHARED_LIBS)
  add_library(${Libname} SHARED)
else()
  add_library(${Libname} STATIC)
endif()

function(add_definition value)
  add_definitions(-D${value})
  target_compile_definitions(${Libname} PUBLIC ${value})
endfunction()

add_definition(ZENOH_C_STANDARD=${CMAKE_C_STANDARD})

# while in development, use timestamp for patch version:
string(TIMESTAMP PROJECT_VERSION_PATCH "%Y%m%ddev")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# Use cmake .. -DCMAKE_BUILD_TYPE=DEBUG for debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)


set(CHECK_THREADS "ON")

# System definition
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_definition(ZENOH_LINUX)
elseif(POSIX_COMPATIBLE)
  add_definition(ZENOH_LINUX)
  set(CHECK_THREADS "OFF")
elseif(CMAKE_SYSTEM_NAME MATCHES "BSD")
  add_definition(ZENOH_BSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  add_definition(ZENOH_MACOS)
  set(MACOSX_RPATH "ON")
elseif(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
  add_definition(ZENOH_EMSCRIPTEN)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  add_definition(ZENOH_WINDOWS)
  add_definition(_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
  if(WITH_ZEPHYR)
    add_definition(ZENOH_ZEPHYR)
  elseif(WITH_FREERTOS_PLUS_TCP)
    add_definition(ZENOH_FREERTOS_PLUS_TCP)
  endif()
else()
  message(FATAL_ERROR "zenoh-pico is not yet available on ${CMAKE_SYSTEM_NAME} platform")
  return()
endif()

# Compiler definition
message("Compilers in use: ${CMAKE_C_COMPILER_ID}, ${CMAKE_CXX_COMPILER_ID}")
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
  add_definition(ZENOH_COMPILER_CLANG)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_definition(ZENOH_COMPILER_GCC)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel" OR CMAKE_C_COMPILER_ID STREQUAL "Intel")
  add_definition(ZENOH_COMPILER_INTEL)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  add_definition(ZENOH_COMPILER_MSVC)
else()
  add_definition(ZENOH_COMPILER_OTHER)
endif()

add_definition(ZENOH_DEBUG=${ZENOH_DEBUG})


# Zenoh pico feature configuration options
set(Z_FEATURE_MULTI_THREAD 1 CACHE STRING "Toggle multithread feature")
set(Z_FEATURE_PUBLICATION 1 CACHE STRING "Toggle publication feature")
set(Z_FEATURE_SUBSCRIPTION 1 CACHE STRING "Toggle subscription feature")
set(Z_FEATURE_QUERY 1 CACHE STRING "Toggle query feature")
set(Z_FEATURE_QUERYABLE 1 CACHE STRING "Toggle queryable feature")
set(Z_FEATURE_RAWETH_TRANSPORT 0 CACHE STRING "Toggle raw ethernet transport feature")
set(Z_FEATURE_INTEREST 1 CACHE STRING "Toggle interest feature")
add_compile_definitions("Z_BUILD_DEBUG=$<CONFIG:Debug>")
message(STATUS "Building with feature confing:\n\
* MULTI-THREAD: ${Z_FEATURE_MULTI_THREAD}\n\
* PUBLICATION: ${Z_FEATURE_PUBLICATION}\n\
* SUBSCRIPTION: ${Z_FEATURE_SUBSCRIPTION}\n\
* QUERY: ${Z_FEATURE_QUERY}\n\
* QUERYABLE: ${Z_FEATURE_QUERYABLE}\n\
* INTEREST: ${Z_FEATURE_INTEREST}\n\
* RAWETH: ${Z_FEATURE_RAWETH_TRANSPORT}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico.h
  @ONLY
)

# Print summary of CMAKE configurations
message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode")
message(STATUS "Build shared library: ${BUILD_SHARED_LIBS}")
message(STATUS "Zenoh Level Log: ${ZENOH_DEBUG}")
if(FRAG_MAX_SIZE)
  message(STATUS "Fragmented message max size: ${FRAG_MAX_SIZE}")
endif()
if(BATCH_UNICAST_SIZE)
  message(STATUS "Unicast batch max size: ${BATCH_UNICAST_SIZE}")
endif()
message(STATUS "Build for Zephyr RTOS: ${WITH_ZEPHYR}")
message(STATUS "Build for FreeRTOS-Plus-TCP: ${WITH_FREERTOS_PLUS_TCP}")
message(STATUS "Configuring for ${CMAKE_SYSTEM_NAME}")

if(SKBUILD)
  set(INSTALL_RPATH "zenoh")
  set(INSTALL_NAME_DIR "zenoh")
  set(INSTALL_INCLUDE_NAME_DIR "zenoh/include")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
if(CHECK_THREADS)
  find_package(Threads REQUIRED)
endif()

if(MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c11 /experimental:c11atomics")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /experimental:c11atomics")
endif()

file(GLOB_RECURSE PublicHeaders
  "include/zenoh-pico/api/*.h"
  "include/zenoh-pico/collections/*.h"
  "include/zenoh-pico/link/*.h"
  "include/zenoh-pico/net/*.h"
  "include/zenoh-pico/protocol/*.h"
  "include/zenoh-pico/session/*.h"
  "include/zenoh-pico/transport/*.h"
  "include/zenoh-pico/utils/*.h"
  "include/zenoh-pico/config.h"
)
target_include_directories(${Libname} PUBLIC ${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE Sources
  "src/api/*.c"
  "src/collections/*.c"
  "src/link/*.c"
  "src/net/*.c"
  "src/protocol/*.c"
  "src/session/*.c"
  "src/transport/*.c"
  "src/utils/*.c"
  "src/system/platform-common.c"
)

if(WITH_ZEPHYR)
  file (GLOB Sources_Zephyr "src/system/zephyr/*.c")
  list(APPEND Sources ${Sources_Zephyr})
elseif(WITH_FREERTOS_PLUS_TCP)
  file (GLOB Sources_Freertos_Plus_TCP "src/system/freertos_plus_tcp/*.c")
  list(APPEND Sources ${Sources_Freertos_Plus_TCP})
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Darwin" OR CMAKE_SYSTEM_NAME MATCHES "BSD" OR POSIX_COMPATIBLE)
  file (GLOB Sources_Unix "src/system/unix/*.c" "src/system/unix/link/*.c")
  list(APPEND Sources ${Sources_Unix})
elseif(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
  file (GLOB Sources_Emscripten "src/system/emscripten/*.c")
  list(APPEND Sources ${Sources_Emscripten})
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  file (GLOB Sources_Windows "src/system/windows/*.c")
  list(APPEND Sources ${Sources_Windows})
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
link_directories(${LIBRARY_OUTPUT_PATH})

target_sources(${Libname} PRIVATE ${Sources})

if(CHECK_THREADS)
  target_link_libraries(${Libname} Threads::Threads)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_libraries(${Libname} rt)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  target_link_libraries(${Libname} Ws2_32 Iphlpapi)
endif()

#
# Build tests, examples, intallation only when project is root
#
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})

option(PACKAGING "Use option on Linux to produce Debian and RPM packages." OFF)
option(BUILD_EXAMPLES "Use this to also build the examples." ON)
option(BUILD_TOOLS "Use this to also build the tools." OFF)
option(BUILD_TESTING "Use this to also build tests." ON)
option(BUILD_INTEGRATION "Use this to also build integration tests." OFF)

message(STATUS "Produce Debian and RPM packages: ${PACKAGING}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Build tools: ${BUILD_TOOLS}")
message(STATUS "Build tests: ${BUILD_TESTING}")
message(STATUS "Build integration: ${BUILD_INTEGRATION}")

install(TARGETS ${Libname}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  COMPONENT Library
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico.h
  DESTINATION include
  COMPONENT Headers
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico
  DESTINATION include
  COMPONENT Headers
)
if(BUILD_SHARED_LIBS)
  set(LIBNAME ${CMAKE_SHARED_LIBRARY_PREFIX}${Libname}${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
  set(LIBNAME ${CMAKE_STATIC_LIBRARY_PREFIX}${Libname}${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/zenohpico")
# Generate <Package>Config.cmake
configure_package_config_file(
  "PackageConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/zenohpicoConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

# Generate <Package>Version.cmake
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/zenohpicoConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/zenohpicoConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/zenohpicoConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
  CONFIGURATIONS ${configurations}
  COMPONENT dev)

if(UNIX)
  configure_file("${CMAKE_SOURCE_DIR}/zenohpico.pc.in" "${CMAKE_SOURCE_DIR}/zenohpico.pc" @ONLY)
  install(FILES "${CMAKE_SOURCE_DIR}/zenohpico.pc" CONFIGURATIONS Release RelWithDebInfo DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(UNIX OR MSVC)
  if(BUILD_TOOLS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools)
    add_executable(z_keyexpr_canonizer ${PROJECT_SOURCE_DIR}/tools/z_keyexpr_canonizer.c)
    target_link_libraries(z_keyexpr_canonizer ${Libname})
  endif()

  if(BUILD_TESTING AND CMAKE_C_STANDARD MATCHES "11")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

    add_executable(z_data_struct_test ${PROJECT_SOURCE_DIR}/tests/z_data_struct_test.c)
    add_executable(z_channels_test ${PROJECT_SOURCE_DIR}/tests/z_channels_test.c)
    add_executable(z_collections_test ${PROJECT_SOURCE_DIR}/tests/z_collections_test.c)
    add_executable(z_endpoint_test ${PROJECT_SOURCE_DIR}/tests/z_endpoint_test.c)
    add_executable(z_iobuf_test ${PROJECT_SOURCE_DIR}/tests/z_iobuf_test.c)
    add_executable(z_msgcodec_test ${PROJECT_SOURCE_DIR}/tests/z_msgcodec_test.c)
    add_executable(z_keyexpr_test ${PROJECT_SOURCE_DIR}/tests/z_keyexpr_test.c)
    add_executable(z_api_null_drop_test ${PROJECT_SOURCE_DIR}/tests/z_api_null_drop_test.c)
    add_executable(z_api_double_drop_test ${PROJECT_SOURCE_DIR}/tests/z_api_double_drop_test.c)
    add_executable(z_test_fragment_tx ${PROJECT_SOURCE_DIR}/tests/z_test_fragment_tx.c)
    add_executable(z_test_fragment_rx ${PROJECT_SOURCE_DIR}/tests/z_test_fragment_rx.c)
    add_executable(z_perf_tx ${PROJECT_SOURCE_DIR}/tests/z_perf_tx.c)
    add_executable(z_perf_rx ${PROJECT_SOURCE_DIR}/tests/z_perf_rx.c)
    add_executable(z_bytes_test ${PROJECT_SOURCE_DIR}/tests/z_bytes_test.c)
    add_executable(z_api_bytes_test ${PROJECT_SOURCE_DIR}/tests/z_api_bytes_test.c)
    add_executable(z_api_encoding_test ${PROJECT_SOURCE_DIR}/tests/z_api_encoding_test.c)
    add_executable(z_api_config_test ${PROJECT_SOURCE_DIR}/tests/z_api_config_test.c)
    add_executable(z_refcount_test ${PROJECT_SOURCE_DIR}/tests/z_refcount_test.c)

    target_link_libraries(z_data_struct_test ${Libname})
    target_link_libraries(z_channels_test ${Libname})
    target_link_libraries(z_collections_test ${Libname})
    target_link_libraries(z_endpoint_test ${Libname})
    target_link_libraries(z_iobuf_test ${Libname})
    target_link_libraries(z_msgcodec_test ${Libname})
    target_link_libraries(z_keyexpr_test ${Libname})
    target_link_libraries(z_api_null_drop_test ${Libname})
    target_link_libraries(z_api_double_drop_test ${Libname})
    target_link_libraries(z_test_fragment_tx ${Libname})
    target_link_libraries(z_test_fragment_rx ${Libname})
    target_link_libraries(z_perf_tx ${Libname})
    target_link_libraries(z_perf_rx ${Libname})
    target_link_libraries(z_bytes_test ${Libname})
    target_link_libraries(z_api_bytes_test ${Libname})
    target_link_libraries(z_api_encoding_test ${Libname})
    target_link_libraries(z_api_config_test ${Libname})
    target_link_libraries(z_refcount_test ${Libname})

    configure_file(${PROJECT_SOURCE_DIR}/tests/modularity.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/modularity.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/raweth.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/raweth.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/fragment.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fragment.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/single_thread.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/single_thread.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/attachment.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/attachment.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/no_router.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/no_router.py COPYONLY)

    enable_testing()
    add_test(z_data_struct_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_data_struct_test)
    add_test(z_channels_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_channels_test)
    add_test(z_collections_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_collections_test)
    add_test(z_endpoint_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_endpoint_test)
    add_test(z_iobuf_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_iobuf_test)
    add_test(z_msgcodec_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_msgcodec_test)
    add_test(z_keyexpr_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_keyexpr_test)
    add_test(z_api_null_drop_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_api_null_drop_test)
    add_test(z_api_double_drop_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_api_double_drop_test)
    add_test(z_bytes_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_bytes_test)
    add_test(z_api_bytes_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_api_bytes_test)
    add_test(z_api_encoding_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_api_encoding_test)
    add_test(z_api_config_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_api_config_test)
    add_test(z_refcount_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_refcount_test)
  endif()

  if(BUILD_MULTICAST)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

    if(CMAKE_C_STANDARD MATCHES "11")
      add_executable(z_peer_multicast_test ${PROJECT_SOURCE_DIR}/tests/z_peer_multicast_test.c)
      target_link_libraries(z_peer_multicast_test ${Libname})

      configure_file(${PROJECT_SOURCE_DIR}/tests/multicast.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/multicast.sh COPYONLY)

      enable_testing()
      add_test(z_peer_multicast_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/multicast.sh z_peer_multicast_test)
    endif()
  endif()

  if(BUILD_INTEGRATION)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

    if(CMAKE_C_STANDARD MATCHES "11")
      add_executable(z_client_test ${PROJECT_SOURCE_DIR}/tests/z_client_test.c)
      add_executable(z_api_alignment_test ${PROJECT_SOURCE_DIR}/tests/z_api_alignment_test.c)
      add_executable(z_session_test ${PROJECT_SOURCE_DIR}/tests/z_session_test.c)

      target_link_libraries(z_client_test ${Libname})
      target_link_libraries(z_api_alignment_test ${Libname})
      target_link_libraries(z_session_test ${Libname})

      configure_file(${PROJECT_SOURCE_DIR}/tests/routed.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/routed.sh COPYONLY)
      configure_file(${PROJECT_SOURCE_DIR}/tests/api.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh COPYONLY)

      enable_testing()
      add_test(z_client_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/routed.sh z_client_test)
      add_test(z_api_alignment_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_alignment_test)
      add_test(z_session_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_session_test)
    endif()
  endif()
endif()

# For packaging
if(PACKAGING)
  set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")

  set(CPACK_COMPONENTS_ALL Library Headers)
  set(CPACK_COMPONENT_LIBRARY_GROUP "lib")
  set(CPACK_COMPONENT_HEADERS_GROUP "dev")
  set(CPACK_COMPONENT_HEADERS_DEPENDS Library)

  set(CPACK_PACKAGE_CHECKSUM MD5)
  set(CPACK_PACKAGE_VENDOR "The Eclipse Foundation")
  set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
  set(CPACK_COMPONENT_LIB_DESCRIPTION "The C client library for Eclipse zenoh targeting pico devices")
  set(CPACK_COMPONENT_DEV_DESCRIPTION "${CPACK_COMPONENT_LIB_DESCRIPTION} - devel files")

  # Sources package
  set(CPACK_SOURCE_GENERATOR "TGZ")
  set(CPACK_SOURCE_IGNORE_FILES "/.git/;/.github/;/build/;/crossbuilds/")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-src-${PROJECT_VERSION}")

  if(PACKAGING MATCHES "DEB")
    if(NOT DEBARCH)
      execute_process(
        COMMAND dpkg --print-architecture
        OUTPUT_VARIABLE DEBARCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    endif()

    message(STATUS "Configure DEB packaging for Linux ${DEBARCH}")

    if(CPACK_GENERATOR)
      set(CPACK_GENERATOR "${CPACK_GENERATOR};DEB")
    else()
      set(CPACK_GENERATOR "DEB")
    endif()

    # DEB package
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ZettaScale Zenoh Team, <zenoh@zettascale.tech>")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${DEBARCH})
    set(CPACK_DEB_COMPONENT_INSTALL ON)
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_LIB_PACKAGE_NAME ${PROJECT_NAME}) # avoid "-lib" suffix for "lib" package
    set(CPACK_DEBIAN_LIB_PACKAGE_DEPENDS "libc6 (>=2.12)")
    set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "${CPACK_DEBIAN_LIB_PACKAGE_NAME} (=${PROJECT_VERSION})")
  endif()

  if(PACKAGING MATCHES "RPM")
    if(NOT RPMARCH)
      set(RPMARCH ${CMAKE_SYSTEM_PROCESSOR})
    endif()

    message(STATUS "Configure RPM packaging for Linux ${RPMARCH}")

    if(CPACK_GENERATOR)
      set(CPACK_GENERATOR "${CPACK_GENERATOR};RPM")
    else()
      set(CPACK_GENERATOR "RPM")
    endif()

    # RPM package
    set(CPACK_RPM_PACKAGE_ARCHITECTURE ${RPMARCH})
    set(CPACK_RPM_COMPONENT_INSTALL ON)
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    set(CPACK_RPM_LIB_PACKAGE_NAME ${PROJECT_NAME}) # avoid "-lib" suffix for "lib" package
    set(CPACK_RPM_DEV_PACKAGE_REQUIRES "${CPACK_RPM_LIB_PACKAGE_NAME} = ${PROJECT_VERSION}")
  endif()

  include(CPack)
endif()

endif()
