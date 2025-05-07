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

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico.h
  @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/library.json.in
  ${CMAKE_CURRENT_SOURCE_DIR}/library.json
  @ONLY
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build shared libraries if ON, otherwise build static libraries" ON)
option(WITH_ZEPHYR "Build for Zephyr RTOS" OFF)
option(WITH_FREERTOS_PLUS_TCP "Build for FreeRTOS RTOS and FreeRTOS-Plus-TCP network stack" OFF)
option(WITH_FREERTOS_LWIP "Build for FreeRTOS RTOS and LWIP (Lightweight IP) network stack" OFF)
option(WITH_RPI_PICO "Build for Raspberry Pi Pico" OFF)
set(ZENOH_LOG "" CACHE STRING "Use this to set the ZENOH_LOG variable")
set(ZENOH_LOG_PRINT "" CACHE STRING "Use this to set the ZENOH_LOG_PRINT variable")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  if (BUILD_SHARED_LIBS) 
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
  if(WITH_ZEPHYR)
    set(PACKAGING OFF) # no packaging support for zephyr
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

# Use cmake .. -DCMAKE_BUILD_TYPE=DEBUG for debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)

# Compile options
if(CMAKE_BUILD_TYPE MATCHES "RELEASE" OR "Release")
  if(UNIX)
    add_compile_options(-pipe -O3)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
    add_compile_options(-pipe -O3)
  endif()
else()
  if(CMAKE_SYSTEM_NAME MATCHES "PICO")
    add_compile_options(-c -Wall -Wextra -Wno-unused -Wno-strict-prototypes -pipe -g -O0)
  elseif(UNIX)
    add_compile_options(-c -Wall -Wextra -Werror -Wshadow -Wunused -Wstrict-prototypes -pipe -g -O0)
    # C99 pedantic doesn't like struct anonymous in unix header
    if (NOT CMAKE_C_STANDARD STREQUAL "99")
      add_compile_options(-Wpedantic)
    endif()
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
      add_compile_options(-Wconversion)
    endif()
  elseif(MSVC)
    add_compile_options(/W4 /WX /Od /wd4127)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wmissing-prototypes -pipe -g -O0)
  endif()
endif()

if (PACKAGING)
  set(PICO_STATIC ON)
  set(PICO_SHARED ON)
endif()
if(BUILD_SHARED_LIBS)
  set(PICO_SHARED ON)
else()
  set(PICO_STATIC ON)
endif()

set(Libname "zenohpico")
if(PICO_STATIC)
  add_library(${Libname}_static STATIC)
  set_target_properties(${Libname}_static PROPERTIES OUTPUT_NAME ${Libname})
  add_library(zenohpico::static ALIAS ${Libname}_static)
endif()
if(PICO_SHARED)
  add_library(${Libname}_shared SHARED)
  set_target_properties(${Libname}_shared PROPERTIES OUTPUT_NAME ${Libname})
  add_library(zenohpico::shared ALIAS ${Libname}_shared)
endif()
if(BUILD_SHARED_LIBS)
  add_library(zenohpico::lib ALIAS ${Libname}_shared)
else()
  add_library(zenohpico::lib ALIAS ${Libname}_static)
endif()

function(pico_add_compile_definition value)
  add_definitions(-D${value})
  if(PICO_STATIC)
    target_compile_definitions(zenohpico_static PUBLIC ${value})
  endif()
  if(PICO_SHARED)
    target_compile_definitions(zenohpico_shared PUBLIC ${value})
  endif()
endfunction()

function(pico_target_link_library value)
  if(PICO_STATIC)
    target_link_libraries(zenohpico_static ${value})
  endif()
  if(PICO_SHARED)
    target_link_libraries(zenohpico_shared ${value})
  endif()
endfunction()

pico_add_compile_definition(ZENOH_C_STANDARD=${CMAKE_C_STANDARD})

if (NOT CMAKE_BUILD_TYPE MATCHES "RELEASE" OR "Release") 
  # while in development, use timestamp for patch version:
  string(TIMESTAMP PROJECT_VERSION_PATCH "%Y%m%ddev")
  set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

set(CHECK_THREADS "ON")

# System definition
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  pico_add_compile_definition(ZENOH_LINUX)
elseif(POSIX_COMPATIBLE)
  pico_add_compile_definition(ZENOH_LINUX)
  set(CHECK_THREADS "OFF")
elseif(CMAKE_SYSTEM_NAME MATCHES "BSD")
  pico_add_compile_definition(ZENOH_BSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  pico_add_compile_definition(ZENOH_MACOS)
  set(MACOSX_RPATH "ON")
elseif(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
  pico_add_compile_definition(ZENOH_EMSCRIPTEN)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  pico_add_compile_definition(ZENOH_WINDOWS)
  pico_add_compile_definition(_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
  if(WITH_ZEPHYR)
    pico_add_compile_definition(ZENOH_ZEPHYR)
  elseif(WITH_FREERTOS_PLUS_TCP)
    pico_add_compile_definition(ZENOH_FREERTOS_PLUS_TCP)
  elseif(WITH_FREERTOS_LWIP)
    pico_add_compile_definition(ZENOH_FREERTOS_LWIP)
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "PICO") # Raspberry Pi Pico Series
  pico_add_compile_definition(ZENOH_RPI_PICO)
  set(CHECK_THREADS "OFF")
else()
  message(FATAL_ERROR "zenoh-pico is not yet available on ${CMAKE_SYSTEM_NAME} platform")
  return()
endif()

# Compiler definition
message("Compilers in use: ${CMAKE_C_COMPILER_ID}, ${CMAKE_CXX_COMPILER_ID}")
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
  pico_add_compile_definition(ZENOH_COMPILER_CLANG)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  pico_add_compile_definition(ZENOH_COMPILER_GCC)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel" OR CMAKE_C_COMPILER_ID STREQUAL "Intel")
  pico_add_compile_definition(ZENOH_COMPILER_INTEL)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  pico_add_compile_definition(ZENOH_COMPILER_MSVC)
else()
  pico_add_compile_definition(ZENOH_COMPILER_OTHER)
endif()

# Logging
if (DEFINED ZENOH_DEBUG AND NOT (ZENOH_DEBUG STREQUAL ""))
  pico_add_compile_definition(ZENOH_DEBUG=${ZENOH_DEBUG})
elseif (ZENOH_LOG STREQUAL ERROR OR ZENOH_LOG STREQUAL error)
  pico_add_compile_definition(ZENOH_LOG_ERROR)
elseif (ZENOH_LOG STREQUAL WARN OR ZENOH_LOG STREQUAL warn)
  pico_add_compile_definition(ZENOH_LOG_WARN)
elseif (ZENOH_LOG STREQUAL INFO OR ZENOH_LOG STREQUAL info)
  pico_add_compile_definition(ZENOH_LOG_INFO)
elseif (ZENOH_LOG STREQUAL DEBUG OR ZENOH_LOG STREQUAL debug)
  pico_add_compile_definition(ZENOH_LOG_DEBUG)
elseif (ZENOH_LOG STREQUAL TRACE OR ZENOH_LOG STREQUAL trace)
  pico_add_compile_definition(ZENOH_LOG_TRACE)
endif()
if (NOT(ZENOH_LOG_PRINT STREQUAL ""))
 pico_add_compile_definition(ZENOH_LOG_PRINT=${ZENOH_LOG_PRINT})
endif()
add_compile_definitions("Z_BUILD_LOG=$<CONFIG:Debug>")

# Zenoh pico feature configuration options
set(FRAG_MAX_SIZE 4096 CACHE STRING "Use this to override the maximum size for fragmented messages")
set(BATCH_UNICAST_SIZE 2048 CACHE STRING "Use this to override the maximum unicast batch size")
set(BATCH_MULTICAST_SIZE 2048 CACHE STRING "Use this to override the maximum multicast batch size")
set(Z_CONFIG_SOCKET_TIMEOUT 100 CACHE STRING "Default socket timeout in milliseconds")
set(Z_TRANSPORT_LEASE 10000 CACHE STRING "Link lease duration in milliseconds to announce to other zenoh nodes")

set(Z_FEATURE_UNSTABLE_API 0 CACHE STRING "Toggle unstable Zenoh-C API")
set(Z_FEATURE_PUBLICATION 1 CACHE STRING "Toggle publication feature")
set(Z_FEATURE_SUBSCRIPTION 1 CACHE STRING "Toggle subscription feature")
set(Z_FEATURE_QUERY 1 CACHE STRING "Toggle query feature")
set(Z_FEATURE_QUERYABLE 1 CACHE STRING "Toggle queryable feature")
set(Z_FEATURE_LIVELINESS 1 CACHE STRING "Toggle liveliness feature")
set(Z_FEATURE_INTEREST 1 CACHE STRING "Toggle interests")
set(Z_FEATURE_FRAGMENTATION 1 CACHE STRING "Toggle fragmentation")
set(Z_FEATURE_ENCODING_VALUES 1 CACHE STRING "Toggle encoding values")
set(Z_FEATURE_MULTI_THREAD 1 CACHE STRING "Toggle multithread")

set(Z_FEATURE_LINK_TCP 1 CACHE STRING "Toggle TCP links")
set(Z_FEATURE_LINK_BLUETOOTH 0 CACHE STRING "Toggle Bluetooth links")
set(Z_FEATURE_LINK_WS 0 CACHE STRING "Toggle WebSocket links")
set(Z_FEATURE_LINK_SERIAL 0 CACHE STRING "Toggle Serial links")
set(Z_FEATURE_LINK_SERIAL_USB 0 CACHE STRING "Toggle Serial USB links")
set(Z_FEATURE_SCOUTING_UDP 1 CACHE STRING "Toggle UDP scouting")
set(Z_FEATURE_LINK_UDP_MULTICAST 1 CACHE STRING "Toggle UDP multicast links")
set(Z_FEATURE_LINK_UDP_UNICAST 1 CACHE STRING "Toggle UDP unicast links")
set(Z_FEATURE_MULTICAST_TRANSPORT 1 CACHE STRING "Toggle multicast transport")
set(Z_FEATURE_UNICAST_TRANSPORT 1 CACHE STRING "Toggle unicast transport")
set(Z_FEATURE_RAWETH_TRANSPORT 0 CACHE STRING "Toggle raw ethernet transport")
set(Z_FEATURE_TCP_NODELAY 1 CACHE STRING "Toggle TCP_NODELAY")
set(Z_FEATURE_LOCAL_SUBSCRIBER 0 CACHE STRING "Toggle local subscriptions")
set(Z_FEATURE_SESSION_CHECK 1 CACHE STRING "Toggle publisher/querier session check")
set(Z_FEATURE_BATCHING 1 CACHE STRING "Toggle batching")
set(Z_FEATURE_MATCHING 1 CACHE STRING "Toggle matching feature")
set(Z_FEATURE_RX_CACHE 0 CACHE STRING "Toggle RX_CACHE")
set(Z_FEATURE_UNICAST_PEER 1 CACHE STRING "Toggle Unicast peer mode")
set(Z_FEATURE_AUTO_RECONNECT 1 CACHE STRING "Toggle automatic reconnection")

# Add a warning message if someone tries to enable Z_FEATURE_LINK_SERIAL_USB directly
if(Z_FEATURE_LINK_SERIAL_USB AND NOT Z_FEATURE_UNSTABLE_API)
  message(WARNING "Z_FEATURE_LINK_SERIAL_USB can only be enabled when Z_FEATURE_UNSTABLE_API is also enabled. Disabling Z_FEATURE_LINK_SERIAL_USB.")
  set(Z_FEATURE_LINK_SERIAL_USB 0 CACHE STRING "Toggle Serial USB links" FORCE)
endif()

if(Z_FEATURE_UNICAST_PEER AND NOT Z_FEATURE_MULTI_THREAD)
  message(WARNING "Z_FEATURE_UNICAST_PEER can only be enabled when Z_FEATURE_MULTI_THREAD is also enabled. Disabling Z_FEATURE_UNICAST_PEER.")
  set(Z_FEATURE_UNICAST_PEER 0 CACHE STRING "Toggle Unicast peer mode" FORCE)
endif()

if(Z_FEATURE_MATCHING AND NOT Z_FEATURE_INTEREST)
  message(STATUS "Z_FEATURE_MATCHING can only be enabled when Z_FEATURE_INTEREST is also enabled. Disabling Z_FEATURE_MATCHING.")
  set(Z_FEATURE_MATCHING 0 CACHE STRING "Toggle matching feature" FORCE)
endif()

if(Z_FEATURE_MATCHING AND NOT Z_FEATURE_UNSTABLE_API)
  message(STATUS "Z_FEATURE_MATCHING disabled because Z_FEATURE_UNSTABLE_API disabled")
  set(Z_FEATURE_MATCHING 0 CACHE STRING "Toggle matching feature" FORCE)
endif()

message(STATUS "Building with feature config:\n\
* UNSTABLE_API: ${Z_FEATURE_UNSTABLE_API}\n\
* MULTI-THREAD: ${Z_FEATURE_MULTI_THREAD}\n\
* PUBLICATION: ${Z_FEATURE_PUBLICATION}\n\
* SUBSCRIPTION: ${Z_FEATURE_SUBSCRIPTION}\n\
* QUERY: ${Z_FEATURE_QUERY}\n\
* QUERYABLE: ${Z_FEATURE_QUERYABLE}\n\
* LIVELINESS: ${Z_FEATURE_LIVELINESS}\n\
* INTEREST: ${Z_FEATURE_INTEREST}\n\
* AUTO_RECONNECT: ${Z_FEATURE_AUTO_RECONNECT}\n\
* MATCHING: ${Z_FEATURE_MATCHING}\n\
* RAWETH: ${Z_FEATURE_RAWETH_TRANSPORT}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico.h
  @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico/config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico/config.h
  @ONLY
)

# Print summary of CMAKE configurations
message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode")
message(STATUS "Build shared library: ${BUILD_SHARED_LIBS}")
message(STATUS "Zenoh Level Log: ${ZENOH_LOG}")
message(STATUS "Fragmented message max size: ${FRAG_MAX_SIZE}")
message(STATUS "Unicast batch max size: ${BATCH_UNICAST_SIZE}")
message(STATUS "Multicast batch max size: ${BATCH_MULTICAST_SIZE}")
message(STATUS "Build for Zephyr RTOS: ${WITH_ZEPHYR}")
message(STATUS "Build for FreeRTOS-Plus-TCP: ${WITH_FREERTOS_PLUS_TCP}")
message(STATUS "Build for FreeRTOS + LWIP: ${WITH_FREERTOS_LWIP}")
message(STATUS "Build for Raspberry Pi Pico: ${WITH_RPI_PICO}")
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
if(PICO_STATIC)
  target_include_directories(${Libname}_static
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>)
endif()
if(PICO_SHARED)
  target_include_directories(${Libname}_shared
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>)
endif()


file(GLOB_RECURSE Sources
  "src/api/*.c"
  "src/collections/*.c"
  "src/link/*.c"
  "src/net/*.c"
  "src/protocol/*.c"
  "src/session/*.c"
  "src/transport/*.c"
  "src/utils/*.c"
  "src/system/common/*.c"
)

if(WITH_ZEPHYR)
  file (GLOB Sources_Zephyr "src/system/zephyr/*.c")
  list(APPEND Sources ${Sources_Zephyr})
elseif(WITH_FREERTOS_PLUS_TCP)
  file (GLOB Sources_Freertos_Plus_TCP "src/system/freertos/freertos_plus_tcp/*.c")
  list(APPEND Sources_Freertos_Plus_TCP "src/system/freertos/system.c")
  list(APPEND Sources ${Sources_Freertos_Plus_TCP})
elseif(WITH_FREERTOS_LWIP)
  file (GLOB Sources_Freertos_Plus_LWIP "src/system/freertos/lwip/*.c")
  list(APPEND Sources_Freertos_Plus_LWIP "src/system/freertos/system.c")
  list(APPEND Sources ${Sources_Freertos_Plus_LWIP})
elseif(WITH_RPI_PICO)
  file (GLOB Sources_RPI_Pico "src/system/rpi_pico/*.c")
  list(APPEND Sources ${Sources_RPI_Pico})
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

if(PICO_STATIC)
  target_sources(zenohpico_static PRIVATE ${Sources})
endif()
if(PICO_SHARED)
  target_sources(zenohpico_shared PRIVATE ${Sources})
endif()

if(CHECK_THREADS)
  pico_target_link_library(Threads::Threads)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  pico_target_link_library(rt)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  pico_target_link_library(Ws2_32)
  pico_target_link_library(Iphlpapi)
endif()

#
# Build tests, examples, installation only when project is root
#
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})

option(PACKAGING "Use option on Linux to produce Debian and RPM packages." OFF)
option(BUILD_EXAMPLES "Use this to also build the examples." ON)
option(BUILD_TOOLS "Use this to also build the tools." OFF)
option(BUILD_TESTING "Use this to also build tests." ON)
option(BUILD_INTEGRATION "Use this to also build integration tests." OFF)
option(ASAN "Enable AddressSanitizer." OFF)

message(STATUS "Produce Debian and RPM packages: ${PACKAGING}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Build tools: ${BUILD_TOOLS}")
message(STATUS "Build tests: ${BUILD_TESTING}")
message(STATUS "Build integration: ${BUILD_INTEGRATION}")
message(STATUS "AddressSanitizer: ${ASAN}")

set(PICO_LIBS "")
if(PICO_STATIC)
  list(APPEND PICO_LIBS zenohpico_static)
endif()
if(PICO_SHARED)
  list(APPEND PICO_LIBS zenohpico_shared)
endif()

install(TARGETS ${PICO_LIBS}
  EXPORT zenohpicoTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  LIBRARY COMPONENT Runtime
  ARCHIVE COMPONENT Dev
  RUNTIME COMPONENT Runtime
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
  COMPONENT Dev)

# Generate <Package>Targets.cmake
install(
  EXPORT zenohpicoTargets
  NAMESPACE zenohpico::
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
  COMPONENT Dev)

if(UNIX)
  configure_file("${CMAKE_SOURCE_DIR}/zenohpico.pc.in" "${CMAKE_SOURCE_DIR}/zenohpico.pc" @ONLY)
  install(FILES "${CMAKE_SOURCE_DIR}/zenohpico.pc" CONFIGURATIONS Release RelWithDebInfo DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig" COMPONENT Dev)
endif()

if(ASAN AND NOT MSVC)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(UNIX OR MSVC)
  if(BUILD_TOOLS)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools)
    add_executable(z_keyexpr_canonizer ${PROJECT_SOURCE_DIR}/tools/z_keyexpr_canonizer.c)
    target_link_libraries(z_keyexpr_canonizer zenohpico::lib)
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
    add_executable(z_refcount_test ${PROJECT_SOURCE_DIR}/tests/z_refcount_test.c)
    add_executable(z_lru_cache_test ${PROJECT_SOURCE_DIR}/tests/z_lru_cache_test.c)
    add_executable(z_test_unicast ${PROJECT_SOURCE_DIR}/tests/z_test_unicast.c)

    target_link_libraries(z_data_struct_test zenohpico::lib)
    target_link_libraries(z_channels_test zenohpico::lib)
    target_link_libraries(z_collections_test zenohpico::lib)
    target_link_libraries(z_endpoint_test zenohpico::lib)
    target_link_libraries(z_iobuf_test zenohpico::lib)
    target_link_libraries(z_msgcodec_test zenohpico::lib)
    target_link_libraries(z_keyexpr_test zenohpico::lib)
    target_link_libraries(z_api_null_drop_test zenohpico::lib)
    target_link_libraries(z_api_double_drop_test zenohpico::lib)
    target_link_libraries(z_test_fragment_tx zenohpico::lib)
    target_link_libraries(z_test_fragment_rx zenohpico::lib)
    target_link_libraries(z_perf_tx zenohpico::lib)
    target_link_libraries(z_perf_rx zenohpico::lib)
    target_link_libraries(z_bytes_test zenohpico::lib)
    target_link_libraries(z_api_bytes_test zenohpico::lib)
    target_link_libraries(z_api_encoding_test zenohpico::lib)
    target_link_libraries(z_refcount_test zenohpico::lib)
    target_link_libraries(z_lru_cache_test zenohpico::lib)
    target_link_libraries(z_test_unicast zenohpico::lib)

    configure_file(${PROJECT_SOURCE_DIR}/tests/modularity.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/modularity.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/raweth.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/raweth.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/fragment.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fragment.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/single_thread.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/single_thread.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/attachment.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/attachment.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/no_router.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/no_router.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/memory_leak.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/memory_leak.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/connection_restore.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/connection_restore.py COPYONLY)

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
    add_test(z_refcount_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_refcount_test)
    add_test(z_lru_cache_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_lru_cache_test)
  endif()

  if(BUILD_MULTICAST)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

    if(CMAKE_C_STANDARD MATCHES "11")
      add_executable(z_peer_multicast_test ${PROJECT_SOURCE_DIR}/tests/z_peer_multicast_test.c)
      target_link_libraries(z_peer_multicast_test zenohpico::lib)

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
      add_executable(z_api_liveliness_test ${PROJECT_SOURCE_DIR}/tests/z_api_liveliness_test.c)
      add_executable(z_api_matching_test ${PROJECT_SOURCE_DIR}/tests/z_api_matching_test.c)
      add_executable(z_api_source_info_test ${PROJECT_SOURCE_DIR}/tests/z_api_source_info_test.c)
      add_executable(z_api_queryable_test ${PROJECT_SOURCE_DIR}/tests/z_api_queryable_test.c)

      target_link_libraries(z_client_test zenohpico::lib)
      target_link_libraries(z_api_alignment_test zenohpico::lib)
      target_link_libraries(z_session_test zenohpico::lib)
      target_link_libraries(z_api_liveliness_test zenohpico::lib)
      target_link_libraries(z_api_matching_test zenohpico::lib)
      target_link_libraries(z_api_source_info_test zenohpico::lib)
      target_link_libraries(z_api_queryable_test zenohpico::lib)

      configure_file(${PROJECT_SOURCE_DIR}/tests/routed.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/routed.sh COPYONLY)
      configure_file(${PROJECT_SOURCE_DIR}/tests/api.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh COPYONLY)

      enable_testing()
      add_test(z_client_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/routed.sh z_client_test)
      add_test(z_api_alignment_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_alignment_test)
      add_test(z_session_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_session_test)
      add_test(z_api_liveliness_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_liveliness_test)
      add_test(z_api_matching_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_matching_test)
      add_test(z_api_source_info_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_source_info_test)
      add_test(z_api_queryable_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_queryable_test)
    endif()
  endif()
endif()

# For packaging
if(PACKAGING)

  set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")

  set(CPACK_COMPONENTS_ALL Runtime Headers Dev)

  set(CPACK_COMPONENT_RUNTIME_GROUP "lib")
  set(CPACK_COMPONENT_HEADERS_GROUP "dev")
  set(CPACK_COMPONENT_DEV_GROUP "dev")
  set(CPACK_COMPONENT_HEADERS_DEPENDS Runtime)
  set(CPACK_COMPONENT_DEV_DEPENDS Runtime)

  set(CPACK_PACKAGE_CHECKSUM MD5)
  set(CPACK_PACKAGE_VENDOR "The Eclipse Foundation")
  if(NOT CPACK_PACKAGE_VERSION)
      set(SEM_VER "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
      if(PROJECT_VERSION_TWEAK STREQUAL "")
          set(CPACK_PACKAGE_VERSION ${SEM_VER})
      elseif(PROJECT_VERSION_TWEAK EQUAL 0)
          set(CPACK_PACKAGE_VERSION "${SEM_VER}~dev-1")
      elseif(PROJECT_VERSION_TWEAK GREATER 0)
          set(CPACK_PACKAGE_VERSION "${SEM_VER}~pre.${PROJECT_VERSION_TWEAK}-1")
      endif()
  endif()
  set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "The C client library for Eclipse zenoh targeting pico devices")
  set(CPACK_COMPONENT_HEADERS_DESCRIPTION "${CPACK_COMPONENT_LIB_DESCRIPTION} - headers")
  set(CPACK_COMPONENT_DEV_DESCRIPTION "${CPACK_COMPONENT_LIB_DESCRIPTION} - config files")

  # Sources package
  set(CPACK_SOURCE_GENERATOR "TGZ")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-src-${project_version}")

  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

  if(PACKAGING MATCHES "DEB")
    if(NOT DEBARCH)
      set(DEBARCH ${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM_NAME})
    endif()

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
    set(CPACK_DEBIAN_LIB_PACKAGE_NAME "lib${CPACK_PACKAGE_NAME}")
    set(CPACK_DEBIAN_LIB_PACKAGE_DEPENDS "libc6 (>=2.12)")
    set(CPACK_DEBIAN_DEV_PACKAGE_NAME "lib${CPACK_PACKAGE_NAME}-dev")
    set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "${CPACK_DEBIAN_LIB_PACKAGE_NAME} (=${CPACK_PACKAGE_VERSION})")
  endif()

  if(PACKAGING MATCHES "RPM")
    if(NOT RPMARCH)
      set(RPMARCH ${CMAKE_SYSTEM_PROCESSOR})
    endif()

    if(CPACK_GENERATOR)
      set(CPACK_GENERATOR "${CPACK_GENERATOR};RPM")
    else()
      set(CPACK_GENERATOR "RPM")
    endif()

    # RPM package
    set(CPACK_RPM_PACKAGE_ARCHITECTURE ${RPMARCH})
    set(CPACK_RPM_COMPONENT_INSTALL ON)
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    set(CPACK_RPM_LIB_PACKAGE_NAME ${CPACK_PACKAGE_NAME}) # avoid "-lib" suffix for "lib" package
    set(CPACK_RPM_DEV_PACKAGE_REQUIRES "${CPACK_RPM_LIB_PACKAGE_NAME} = ${CPACK_PACKAGE_VERSION}")
  endif()

  include(CPack)
endif()

endif()
