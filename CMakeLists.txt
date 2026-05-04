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
cmake_minimum_required(VERSION 3.14)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/version.txt version)

project(zenohpico VERSION ${version} LANGUAGES C)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(helpers)
include(platforms)

# Configure header file to define the project version
set(ZENOH_PICO ${PROJECT_VERSION})
set(ZENOH_PICO_MAJOR ${PROJECT_VERSION_MAJOR})
set(ZENOH_PICO_MINOR ${PROJECT_VERSION_MINOR})
set(ZENOH_PICO_PATCH ${PROJECT_VERSION_PATCH})
if(PROJECT_VERSION_TWEAK STREQUAL "")
  set(ZENOH_PICO_TWEAK 0)
else()
  set(ZENOH_PICO_TWEAK ${PROJECT_VERSION_TWEAK})
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico.h
  @ONLY
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build shared libraries if ON, otherwise build static libraries" ON)
set(ZP_PLATFORM "" CACHE STRING "Built-in or external platform profile")
set(ZP_EXTERNAL_PACKAGES "" CACHE STRING "Optional external CMake packages to load")
set(ZENOH_LOG "" CACHE STRING "Use this to set the ZENOH_LOG variable")
set(ZENOH_LOG_PRINT "" CACHE STRING "Use this to set the ZENOH_LOG_PRINT variable")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

set(ZP_PLATFORM_DIRS "")
set(ZP_EXTERNAL_IMPORTED_TARGETS "")
set(ZP_EXTERNAL_IMPORTED_TARGET_PACKAGES "")
if(NOT ZP_EXTERNAL_PACKAGES STREQUAL "")
  foreach(_zp_external_package IN LISTS ZP_EXTERNAL_PACKAGES)
    zp_find_external_package("${_zp_external_package}")
  endforeach()
endif()
find_package(zenohpico_platform CONFIG REQUIRED CONFIGS platformConfig.cmake PATHS "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
             NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
if(DEFINED ZP_SYSTEM_LAYER AND NOT "${ZP_SYSTEM_LAYER}" STREQUAL "")
  message(FATAL_ERROR "ZP_SYSTEM_LAYER is no longer supported. Use ZP_PLATFORM instead.")
endif()
if(ZP_PLATFORM STREQUAL "")
  zp_detect_default_platform(_zp_default_platform)
  set(ZP_PLATFORM "${_zp_default_platform}")
endif()
if(ZP_PLATFORM STREQUAL "")
  message(FATAL_ERROR "No platform profile selected. Set ZP_PLATFORM.")
endif()
if(DEFINED Z_FEATURE_LINK_UDP_MULTICAST)
  set(_zp_link_udp_multicast "${Z_FEATURE_LINK_UDP_MULTICAST}")
else()
  set(_zp_link_udp_multicast "1")
endif()
if(DEFINED Z_FEATURE_MULTICAST_TRANSPORT)
  set(_zp_multicast_transport "${Z_FEATURE_MULTICAST_TRANSPORT}")
else()
  set(_zp_multicast_transport "1")
endif()
if("${_zp_link_udp_multicast}" STREQUAL "1" AND "${_zp_multicast_transport}" STREQUAL "1")
  set(ZP_UDP_MULTICAST_ENABLED ON)
else()
  set(ZP_UDP_MULTICAST_ENABLED OFF)
endif()
zp_load_platform_profile("${ZP_PLATFORM}")
if(NOT ZP_PLATFORM_SYSTEM_LAYER STREQUAL "")
  set(ZP_SYSTEM_LAYER "${ZP_PLATFORM_SYSTEM_LAYER}")
else()
  set(ZP_SYSTEM_LAYER "${ZP_PLATFORM}")
endif()
set(ZP_SYSTEM_PLATFORM_HEADER "${ZP_PLATFORM_SYSTEM_PLATFORM_HEADER}")

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  if (BUILD_SHARED_LIBS) 
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
  if(ZP_SYSTEM_LAYER STREQUAL "zephyr")
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
    target_link_libraries(zenohpico_static PUBLIC ${value})
  endif()
  if(PICO_SHARED)
    target_link_libraries(zenohpico_shared PRIVATE ${value})
  endif()
endfunction()

pico_add_compile_definition(ZENOH_C_STANDARD=${CMAKE_C_STANDARD})

if (NOT CMAKE_BUILD_TYPE MATCHES "RELEASE" OR "Release") 
  # while in development, use timestamp for patch version:
  string(TIMESTAMP PROJECT_VERSION_PATCH "%Y%m%ddev")
  set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

if(ZP_SYSTEM_LAYER STREQUAL "macos")
  set(MACOSX_RPATH "ON")
endif()

set(_zp_platform_globbed_sources "")
foreach(_zp_platform_source_glob IN LISTS ZP_PLATFORM_SOURCE_GLOBS)
  file(GLOB _zp_globbed_platform_sources ${_zp_platform_source_glob})
  list(APPEND _zp_platform_globbed_sources ${_zp_globbed_platform_sources})
endforeach()
set(_zp_platform_sources
    ${_zp_platform_globbed_sources}
    ${ZP_PLATFORM_SOURCE_FILES})
set(_zp_platform_include_dirs ${ZP_PLATFORM_INCLUDE_DIRS})
set(_zp_platform_compile_definitions ${ZP_PLATFORM_COMPILE_DEFINITIONS})
set(_zp_platform_compile_options ${ZP_PLATFORM_COMPILE_OPTIONS})
set(_zp_platform_link_libraries ${ZP_PLATFORM_LINK_LIBRARIES})
set(_zp_platform_imported_targets "")
foreach(_zp_platform_target IN LISTS _zp_platform_link_libraries)
  if(NOT TARGET "${_zp_platform_target}")
    continue()
  endif()

  get_property(_zp_is_imported TARGET "${_zp_platform_target}" PROPERTY IMPORTED)
  if(NOT _zp_is_imported)
    message(FATAL_ERROR
            "Platform from ${ZP_PLATFORM_PROFILE_FILE} requires "
            "ZP_PLATFORM_LINK_LIBRARIES targets to be imported/exported package targets. "
            "Local targets created directly inside package config files are not supported "
            "because they break static install/export paths.")
  endif()

  get_target_property(_zp_platform_target_type "${_zp_platform_target}" TYPE)
  if(_zp_platform_target_type STREQUAL "INTERFACE_LIBRARY")
    get_target_property_if_set(_zp_interface_sources "${_zp_platform_target}" INTERFACE_SOURCES)
    if(NOT "${_zp_interface_sources}" STREQUAL "")
      message(FATAL_ERROR
              "External platform target ${_zp_platform_target} must not use INTERFACE_SOURCES. "
              "Define a real library target instead.")
    endif()
  endif()

  list(APPEND _zp_platform_imported_targets "${_zp_platform_target}")
endforeach()
list(REMOVE_DUPLICATES _zp_platform_imported_targets)
zp_source_list_requires_cxx(_zp_platform_uses_cxx ${_zp_platform_sources})
if(_zp_platform_uses_cxx)
  set(ZP_USES_CXX ON)
else()
  set(ZP_USES_CXX OFF)
  foreach(_zp_platform_target IN LISTS _zp_platform_imported_targets)
    zp_target_requires_cxx("${_zp_platform_target}" _zp_platform_target_uses_cxx)
    if(_zp_platform_target_uses_cxx)
      set(ZP_USES_CXX ON)
      break()
    endif()
  endforeach()
endif()
if(ZP_USES_CXX)
  enable_language(CXX)
endif()
foreach(_zp_platform_definition IN LISTS _zp_platform_compile_definitions)
  pico_add_compile_definition(${_zp_platform_definition})
endforeach()

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
set(Z_TRANSPORT_LEASE_EXPIRE_FACTOR 3 CACHE STRING "Default session lease expire factor.")
set(Z_RUNTIME_MAX_TASKS 64 CACHE STRING "Maximum number of tasks in zenoh-pico's runtime")
set(Z_TRANSPORT_ACCEPT_TIMEOUT 1000 CACHE STRING "Link accept timeout in P2P mode in milliseconds")
set(Z_TRANSPORT_CONNECT_TIMEOUT 10000 CACHE STRING "Link connect timeout in P2P mode inmilliseconds")

set(Z_FEATURE_UNSTABLE_API 0 CACHE STRING "Toggle unstable Zenoh-C API")
set(Z_FEATURE_CONNECTIVITY 0 CACHE STRING "Toggle connectivity status/events API (unstable)")
set(Z_FEATURE_PUBLICATION 1 CACHE STRING "Toggle publication feature")
set(Z_FEATURE_ADVANCED_PUBLICATION 0 CACHE STRING "Toggle advanced publication feature")
set(Z_FEATURE_SUBSCRIPTION 1 CACHE STRING "Toggle subscription feature")
set(Z_FEATURE_ADVANCED_SUBSCRIPTION 0 CACHE STRING "Toggle advanced subscription feature")
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
set(Z_FEATURE_LINK_TLS 0 CACHE STRING "Toggle TLS links")
set(Z_FEATURE_SCOUTING 1 CACHE STRING "Toggle UDP scouting")
set(Z_FEATURE_LINK_UDP_MULTICAST 1 CACHE STRING "Toggle UDP multicast links")
set(Z_FEATURE_LINK_UDP_UNICAST 1 CACHE STRING "Toggle UDP unicast links")
set(Z_FEATURE_MULTICAST_TRANSPORT 1 CACHE STRING "Toggle multicast transport")
set(Z_FEATURE_UNICAST_TRANSPORT 1 CACHE STRING "Toggle unicast transport")
set(Z_FEATURE_RAWETH_TRANSPORT 0 CACHE STRING "Toggle raw ethernet transport")
set(Z_FEATURE_TCP_NODELAY 1 CACHE STRING "Toggle TCP_NODELAY")
set(Z_FEATURE_LOCAL_SUBSCRIBER 0 CACHE STRING "Toggle local subscriptions")
set(Z_FEATURE_SESSION_CHECK 1 CACHE STRING "Toggle publisher/querier session check")
set(Z_FEATURE_BATCHING 1 CACHE STRING "Toggle batching")
set(Z_FEATURE_BATCH_TX_MUTEX 0 CACHE STRING "Toggle tx mutex lock at a batch level")
set(Z_FEATURE_BATCH_PEER_MUTEX 0 CACHE STRING "Toggle peer mutex lock at a batch level")
set(Z_FEATURE_MATCHING 1 CACHE STRING "Toggle matching feature")
set(Z_FEATURE_RX_CACHE 0 CACHE STRING "Toggle RX_CACHE")
set(Z_FEATURE_UNICAST_PEER 1 CACHE STRING "Toggle Unicast peer mode")
set(Z_FEATURE_AUTO_RECONNECT 1 CACHE STRING "Toggle automatic reconnection")
set(Z_FEATURE_MULTICAST_DECLARATIONS 0 CACHE STRING "Toggle multicast resource declarations")
set(Z_FEATURE_LOCAL_QUERYABLE 0 CACHE STRING "Toggle local queriables")
set(Z_FEATURE_ADMIN_SPACE 0 CACHE STRING "Toggle admin space support")

# Add a warning message if someone tries to enable Z_FEATURE_LINK_SERIAL_USB directly
if(Z_FEATURE_LINK_SERIAL_USB AND NOT Z_FEATURE_UNSTABLE_API)
  message(WARNING "Z_FEATURE_LINK_SERIAL_USB can only be enabled when Z_FEATURE_UNSTABLE_API is also enabled. Disabling Z_FEATURE_LINK_SERIAL_USB.")
  set(Z_FEATURE_LINK_SERIAL_USB 0 CACHE STRING "Toggle Serial USB links" FORCE)
endif()

if(Z_FEATURE_LINK_WS AND NOT ZP_SYSTEM_LAYER STREQUAL "emscripten")
  message(FATAL_ERROR "Z_FEATURE_LINK_WS is currently only supported on the emscripten platform.")
endif()

if(Z_FEATURE_CONNECTIVITY AND NOT Z_FEATURE_UNSTABLE_API)
  message(WARNING "Z_FEATURE_CONNECTIVITY can only be enabled when Z_FEATURE_UNSTABLE_API is also enabled. Disabling Z_FEATURE_CONNECTIVITY.")
  set(Z_FEATURE_CONNECTIVITY 0 CACHE STRING "Toggle connectivity status/events API (unstable)" FORCE)
endif()

if(Z_FEATURE_MATCHING AND NOT Z_FEATURE_INTEREST)
  message(STATUS "Z_FEATURE_MATCHING can only be enabled when Z_FEATURE_INTEREST is also enabled. Disabling Z_FEATURE_MATCHING.")
  set(Z_FEATURE_MATCHING 0 CACHE STRING "Toggle matching feature" FORCE)
endif()

if(Z_FEATURE_SCOUTING AND NOT Z_FEATURE_LINK_UDP_UNICAST)
  message(STATUS "Z_FEATURE_SCOUTING disabled because Z_FEATURE_LINK_UDP_UNICAST disabled")
  set(Z_FEATURE_SCOUTING 0 CACHE STRING "Toggle scouting feature" FORCE)
endif()

if(ZP_SYSTEM_LAYER STREQUAL "freertos_plus_tcp" AND Z_FEATURE_LINK_UDP_MULTICAST)
  message(STATUS "Z_FEATURE_LINK_UDP_MULTICAST disabled for FreeRTOS-Plus-TCP system layer")
  set(Z_FEATURE_LINK_UDP_MULTICAST 0 CACHE STRING "Toggle UDP multicast links" FORCE)
endif()

if(Z_FEATURE_ADVANCED_PUBLICATION AND (NOT Z_FEATURE_UNSTABLE_API OR NOT Z_FEATURE_PUBLICATION OR NOT Z_FEATURE_LIVELINESS))
  message(WARNING "Z_FEATURE_ADVANCED_PUBLICATION can only be enabled when Z_FEATURE_UNSTABLE_API, Z_FEATURE_PUBLICATION and Z_FEATURE_LIVELINESS is also enabled. Disabling Z_FEATURE_ADVANCED_PUBLICATION.")
  set(Z_FEATURE_ADVANCED_PUBLICATION 0 CACHE STRING "Toggle advanced publication feature" FORCE)
endif()


if(Z_FEATURE_ADMIN_SPACE AND NOT Z_FEATURE_UNSTABLE_API)
  message(WARNING "Z_FEATURE_ADMIN_SPACE can only be enabled when Z_FEATURE_UNSTABLE_API is enabled. Disabling Z_FEATURE_ADMIN_SPACE.")
  set(Z_FEATURE_ADMIN_SPACE 0 CACHE STRING "Toggle admin space support" FORCE)
endif()

if(Z_FEATURE_ADMIN_SPACE AND NOT Z_FEATURE_QUERYABLE)
  message(WARNING "Z_FEATURE_ADMIN_SPACE can only be enabled when Z_FEATURE_QUERYABLE is enabled. Disabling Z_FEATURE_ADMIN_SPACE.")
  set(Z_FEATURE_ADMIN_SPACE 0 CACHE STRING "Toggle admin space support" FORCE)
endif()


message(STATUS "Building with feature config:\n\
* UNSTABLE_API: ${Z_FEATURE_UNSTABLE_API}\n\
* CONNECTIVITY: ${Z_FEATURE_CONNECTIVITY}\n\
* MULTI-THREAD: ${Z_FEATURE_MULTI_THREAD}\n\
* PUBLICATION: ${Z_FEATURE_PUBLICATION}\n\
* SUBSCRIPTION: ${Z_FEATURE_SUBSCRIPTION}\n\
* ADVANCED PUBLICATION: ${Z_FEATURE_ADVANCED_PUBLICATION}\n\
* ADVANCED SUBSCRIPTION: ${Z_FEATURE_ADVANCED_SUBSCRIPTION}\n\
* QUERY: ${Z_FEATURE_QUERY}\n\
* QUERYABLE: ${Z_FEATURE_QUERYABLE}\n\
* LIVELINESS: ${Z_FEATURE_LIVELINESS}\n\
* INTEREST: ${Z_FEATURE_INTEREST}\n\
* AUTO_RECONNECT: ${Z_FEATURE_AUTO_RECONNECT}\n\
* MATCHING: ${Z_FEATURE_MATCHING}\n\
* RAWETH: ${Z_FEATURE_RAWETH_TRANSPORT}\n\
* ADMIN SPACE: ${Z_FEATURE_ADMIN_SPACE}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/zenoh-pico/config.h
  @ONLY
)

# Print summary of CMAKE configurations
message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode")
message(STATUS "Build shared library: ${BUILD_SHARED_LIBS}")
message(STATUS "Zenoh Level Log: ${ZENOH_LOG}")
message(STATUS "Fragmented message max size: ${FRAG_MAX_SIZE}")
message(STATUS "Unicast batch max size: ${BATCH_UNICAST_SIZE}")
message(STATUS "Multicast batch max size: ${BATCH_MULTICAST_SIZE}")
if(NOT ZP_PLATFORM STREQUAL "")
  message(STATUS "Platform profile: ${ZP_PLATFORM}")
else()
  message(STATUS "Platform profile: none")
endif()
message(STATUS "System layer: ${ZP_SYSTEM_LAYER}")
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

if(PICO_STATIC)
  target_include_directories(${Libname}_static
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>)
endif()
if(PICO_SHARED)
  target_include_directories(${Libname}_shared
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>)
endif()

# TLS support via Mbed TLS
if(Z_FEATURE_LINK_TLS)
  find_package(PkgConfig REQUIRED)

  # Prefer Mbed TLS 3.x, fall back to 2.x
  # Never accept 4.x or newer
  #
  pkg_search_module(MBEDTLS REQUIRED
    mbedtls-3>=3.0.0
    mbedtls>=3.0.0
    mbedtls>=2.0.0
  )
  if(NOT MBEDTLS_VERSION VERSION_LESS "4.0.0")
    message(FATAL_ERROR
      "Mbed TLS ${MBEDTLS_VERSION} is not supported. "
      "Please install Mbed TLS 2.x or 3.x.")
  endif()
  message(STATUS "Using Mbed TLS ${MBEDTLS_VERSION}")

  pkg_search_module(MBEDX509 OPTIONAL
    mbedx509-3>=3.0.0
    mbedx509>=3.0.0
    mbedx509>=2.0.0)

  pkg_search_module(MBEDCRYPTO OPTIONAL
    mbedcrypto-3>=3.0.0
    mbedcrypto>=3.0.0
    mbedcrypto>=2.0.0)

  set(MBEDTLS_LINK_LIBRARIES
    ${MBEDTLS_LIBRARIES}
  )

  if(MBEDX509_FOUND)
    if(MBEDX509_VERSION AND NOT MBEDX509_VERSION VERSION_EQUAL MBEDTLS_VERSION)
      message(FATAL_ERROR
        "mbedtls (${MBEDTLS_VERSION}) and mbedx509 (${MBEDX509_VERSION}) "
        "versions do not match.")
    endif()
    list(APPEND MBEDTLS_LINK_LIBRARIES ${MBEDX509_LIBRARIES})
  endif()

  if(MBEDCRYPTO_FOUND)
    if(MBEDCRYPTO_VERSION AND NOT MBEDCRYPTO_VERSION VERSION_EQUAL MBEDTLS_VERSION)
      message(FATAL_ERROR
        "mbedtls (${MBEDTLS_VERSION}) and mbedcrypto (${MBEDCRYPTO_VERSION}) "
        "versions do not match.")
    endif()
    list(APPEND MBEDTLS_LINK_LIBRARIES ${MBEDCRYPTO_LIBRARIES})
  endif()

  if(PICO_STATIC)
    target_include_directories(${Libname}_static PUBLIC ${MBEDTLS_INCLUDE_DIRS})
    target_link_directories(${Libname}_static PUBLIC ${MBEDTLS_LIBRARY_DIRS})
    target_link_libraries(${Libname}_static PUBLIC ${MBEDTLS_LINK_LIBRARIES})
  endif()
  if(PICO_SHARED)
    target_include_directories(${Libname}_shared PUBLIC ${MBEDTLS_INCLUDE_DIRS})
    target_link_directories(${Libname}_shared PUBLIC ${MBEDTLS_LIBRARY_DIRS})
    target_link_libraries(${Libname}_shared PUBLIC ${MBEDTLS_LINK_LIBRARIES})
  endif()
endif()

file(GLOB_RECURSE Sources
  "src/api/*.c"
  "src/collections/*.c"
  "src/net/*.c"
  "src/protocol/*.c"
  "src/runtime/*.c"
  "src/session/*.c"
  "src/transport/*.c"
  "src/utils/*.c"
  "src/system/common/*.c"
)
file(GLOB LinkSources
  "src/link/config/*.c"
  "src/link/multicast/*.c"
  "src/link/unicast/*.c"
  "src/link/transport/common/*.c"
)
list(APPEND Sources ${LinkSources})
list(APPEND Sources
  "${PROJECT_SOURCE_DIR}/src/link/link.c"
  "${PROJECT_SOURCE_DIR}/src/link/endpoint.c"
  "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/address.c"
  "${PROJECT_SOURCE_DIR}/src/link/transport/udp/address.c"
  "${PROJECT_SOURCE_DIR}/src/link/transport/upper/serial_protocol.c"
  "${PROJECT_SOURCE_DIR}/src/link/transport/upper/tls_stream.c"
)
list(APPEND Sources ${_zp_platform_sources})

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
link_directories(${LIBRARY_OUTPUT_PATH})

if(PICO_STATIC)
  target_sources(zenohpico_static PRIVATE ${Sources})
endif()
if(PICO_SHARED)
  target_sources(zenohpico_shared PRIVATE ${Sources})
endif()
list(REMOVE_DUPLICATES _zp_platform_include_dirs)
if(NOT "${_zp_platform_include_dirs}" STREQUAL "")
  set(_zp_build_interface_platform_include_dirs "")
  foreach(_zp_platform_include_dir IN LISTS _zp_platform_include_dirs)
    list(APPEND _zp_build_interface_platform_include_dirs
         "$<BUILD_INTERFACE:${_zp_platform_include_dir}>")
  endforeach()
  if(PICO_STATIC)
    target_include_directories(zenohpico_static PUBLIC ${_zp_build_interface_platform_include_dirs})
  endif()
  if(PICO_SHARED)
    target_include_directories(zenohpico_shared PUBLIC ${_zp_build_interface_platform_include_dirs})
  endif()
endif()
list(REMOVE_DUPLICATES _zp_platform_compile_definitions)
foreach(_zp_platform_definition IN LISTS _zp_platform_compile_definitions)
  pico_add_compile_definition(${_zp_platform_definition})
endforeach()
list(REMOVE_DUPLICATES _zp_platform_compile_options)
if(NOT "${_zp_platform_compile_options}" STREQUAL "")
  if(PICO_STATIC)
    target_compile_options(zenohpico_static PRIVATE ${_zp_platform_compile_options})
  endif()
  if(PICO_SHARED)
    target_compile_options(zenohpico_shared PRIVATE ${_zp_platform_compile_options})
  endif()
endif()
list(REMOVE_DUPLICATES _zp_platform_link_libraries)
foreach(_zp_platform_link_library IN LISTS _zp_platform_link_libraries)
  pico_target_link_library("${_zp_platform_link_library}")
endforeach()

if(CHECK_THREADS)
  pico_target_link_library(Threads::Threads)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  pico_target_link_library(rt)
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

set(_zp_required_dependency_targets "")
if(PICO_STATIC)
  list(APPEND _zp_required_dependency_targets ${_zp_platform_imported_targets})
endif()
list(REMOVE_DUPLICATES _zp_required_dependency_targets)
zp_collect_target_packages(ZP_PACKAGE_DEPENDENCY_PACKAGES
                           ${_zp_required_dependency_targets})

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
  PATTERN "include/zenoh-pico/config.h" EXCLUDE
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/zenoh-pico/config.h
  DESTINATION include/zenoh-pico
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
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/zenohpico.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/zenohpico.pc" @ONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/zenohpico.pc" CONFIGURATIONS Release RelWithDebInfo DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig" COMPONENT Dev)
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
    add_executable(z_test_peer_unicast ${PROJECT_SOURCE_DIR}/tests/z_test_peer_unicast.c)
    add_executable(z_test_peer_multicast ${PROJECT_SOURCE_DIR}/tests/z_test_peer_multicast.c)
    add_executable(z_utils_test ${PROJECT_SOURCE_DIR}/tests/z_utils_test.c)
    add_executable(z_tls_test ${PROJECT_SOURCE_DIR}/tests/z_tls_test.c)
    add_executable(z_tls_config_test ${PROJECT_SOURCE_DIR}/tests/z_tls_config_test.c)
    add_executable(z_condvar_wait_until_test ${PROJECT_SOURCE_DIR}/tests/z_condvar_wait_until_test.c)
    add_executable(z_sync_group_test ${PROJECT_SOURCE_DIR}/tests/z_sync_group_test.c)
    add_executable(z_cancellation_token_test ${PROJECT_SOURCE_DIR}/tests/z_cancellation_token_test.c)
    add_executable(z_local_loopback_test ${PROJECT_SOURCE_DIR}/tests/z_local_loopback_test.c)
    add_executable(z_json_encoder_test ${PROJECT_SOURCE_DIR}/tests/z_json_encoder_test.c)
    add_executable(z_executor_test ${PROJECT_SOURCE_DIR}/tests/z_executor_test.c)
    add_executable(z_background_executor_test ${PROJECT_SOURCE_DIR}/tests/z_background_executor_test.c)
    add_executable(z_hashmap_test ${PROJECT_SOURCE_DIR}/tests/z_hashmap_test.c)
    add_executable(z_pqueue_test ${PROJECT_SOURCE_DIR}/tests/z_pqueue_test.c)
    add_executable(z_test_fragment_decode_error_transport_zbuf ${PROJECT_SOURCE_DIR}/tests/z_test_fragment_decode_error_transport_zbuf.c)

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
    target_link_libraries(z_test_peer_unicast zenohpico::lib)
    target_link_libraries(z_test_peer_multicast zenohpico::lib)
    target_link_libraries(z_utils_test zenohpico::lib)
    target_link_libraries(z_tls_test zenohpico::lib)
    target_link_libraries(z_tls_config_test zenohpico::lib)
    target_link_libraries(z_condvar_wait_until_test zenohpico::lib)
    target_link_libraries(z_sync_group_test zenohpico::lib)
    target_link_libraries(z_cancellation_token_test zenohpico::lib)
    target_link_libraries(z_local_loopback_test zenohpico::lib)
    target_compile_definitions(z_local_loopback_test PRIVATE Z_TEST_HOOKS=1)
    if(PICO_SHARED)
      target_compile_definitions(${Libname}_shared PRIVATE Z_TEST_HOOKS=1)
    endif()
    if(PICO_STATIC)
      target_compile_definitions(${Libname}_static PRIVATE Z_TEST_HOOKS=1)
    endif()
    target_link_libraries(z_json_encoder_test zenohpico::lib)
    target_link_libraries(z_executor_test zenohpico::lib)
    target_link_libraries(z_background_executor_test zenohpico::lib)
    target_link_libraries(z_hashmap_test zenohpico::lib)
    target_link_libraries(z_pqueue_test zenohpico::lib)
    target_link_libraries(z_test_fragment_decode_error_transport_zbuf zenohpico::lib)
    target_compile_definitions(z_test_fragment_decode_error_transport_zbuf PRIVATE Z_TEST_HOOKS=1)

    configure_file(${PROJECT_SOURCE_DIR}/tests/modularity.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/modularity.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/raweth.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/raweth.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/fragment.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fragment.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/single_thread.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/single_thread.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/attachment.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/attachment.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/no_router.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/no_router.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/memory_leak.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/memory_leak.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/connection_restore.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/connection_restore.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/routed_peer_client.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/routed_peer_client.py COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/valgrind.supp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/valgrind.supp COPYONLY)
    if(UNIX)
      configure_file(${PROJECT_SOURCE_DIR}/tests/package_mylinux.sh.in
                     ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package_mylinux.sh @ONLY)
      configure_file(${PROJECT_SOURCE_DIR}/tests/package_myrtos.sh.in
                     ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package_myrtos.sh @ONLY)
    endif()

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
    add_test(z_utils_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_utils_test)
    add_test(z_tls_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_tls_test)
    add_test(z_tls_config_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_tls_config_test)
    add_test(z_condvar_wait_until_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_condvar_wait_until_test)
    add_test(z_sync_group_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_sync_group_test)
    add_test(z_cancellation_token_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_cancellation_token_test)
    add_test(z_local_loopback_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_local_loopback_test)
    add_test(z_json_encoder_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_json_encoder_test)
    add_test(z_executor_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_executor_test)
    add_test(z_background_executor_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_background_executor_test)
    add_test(z_hashmap_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_hashmap_test)
    add_test(z_pqueue_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_pqueue_test)
    add_test(z_test_fragment_decode_error_transport_zbuf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_test_fragment_decode_error_transport_zbuf)
    if(UNIX)
      add_test(z_package_mylinux_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package_mylinux.sh)
      add_test(z_package_myrtos_configure_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package_myrtos.sh)
    endif()
  endif()

  if(BUILD_INTEGRATION)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

    if(CMAKE_C_STANDARD MATCHES "11")
      add_executable(z_client_test ${PROJECT_SOURCE_DIR}/tests/z_client_test.c)
      add_executable(z_api_alignment_test ${PROJECT_SOURCE_DIR}/tests/z_api_alignment_test.c)
      add_executable(z_wildcard_subscription_test ${PROJECT_SOURCE_DIR}/tests/z_wildcard_subscription_test.c)
      add_executable(z_api_liveliness_test ${PROJECT_SOURCE_DIR}/tests/z_api_liveliness_test.c)
      add_executable(z_api_matching_test ${PROJECT_SOURCE_DIR}/tests/z_api_matching_test.c)
      add_executable(z_api_source_info_test ${PROJECT_SOURCE_DIR}/tests/z_api_source_info_test.c)
      add_executable(z_api_connectivity_test ${PROJECT_SOURCE_DIR}/tests/z_api_connectivity_test.c)
      add_executable(z_api_queryable_test ${PROJECT_SOURCE_DIR}/tests/z_api_queryable_test.c)
      add_executable(z_api_cancellation_test ${PROJECT_SOURCE_DIR}/tests/z_api_cancellation_test.c)
      add_executable(z_api_local_queryable_test ${PROJECT_SOURCE_DIR}/tests/z_api_local_queryable_test.c)
      add_executable(z_api_local_subscriber_test ${PROJECT_SOURCE_DIR}/tests/z_api_local_subscriber_test.c)
      add_executable(z_api_admin_space_test ${PROJECT_SOURCE_DIR}/tests/z_api_admin_space_test.c)
      add_executable(z_multi_pubsub_test ${PROJECT_SOURCE_DIR}/tests/z_multi_pubsub_test.c)
      add_executable(z_multi_queryable_test ${PROJECT_SOURCE_DIR}/tests/z_multi_queryable_test.c)
      add_executable(z_api_batching_test ${PROJECT_SOURCE_DIR}/tests/z_api_batching_test.c)
if(UNIX)
      add_executable(z_api_advanced_pubsub_test ${PROJECT_SOURCE_DIR}/tests/z_api_advanced_pubsub_test.c
                                                ${PROJECT_SOURCE_DIR}/tests/utils/tcp_proxy.c)
      add_executable(z_api_callback_drop_on_undeclare_test ${PROJECT_SOURCE_DIR}/tests/z_api_callback_drop_on_undeclare_test.c
                                                ${PROJECT_SOURCE_DIR}/tests/utils/tcp_proxy.c)
      add_compile_definitions(Z_ADVANCED_PUBSUB_TEST_USE_TCP_PROXY)
else()
      add_executable(z_api_advanced_pubsub_test ${PROJECT_SOURCE_DIR}/tests/z_api_advanced_pubsub_test.c)
      add_executable(z_api_callback_drop_on_undeclare_test ${PROJECT_SOURCE_DIR}/tests/z_api_callback_drop_on_undeclare_test.c)
endif()

      target_link_libraries(z_client_test zenohpico::lib)
      target_link_libraries(z_api_alignment_test zenohpico::lib)
      target_link_libraries(z_wildcard_subscription_test zenohpico::lib)
      target_link_libraries(z_api_liveliness_test zenohpico::lib)
      target_link_libraries(z_api_matching_test zenohpico::lib)
      target_link_libraries(z_api_source_info_test zenohpico::lib)
      target_link_libraries(z_api_connectivity_test zenohpico::lib)
      target_link_libraries(z_api_queryable_test zenohpico::lib)
      target_link_libraries(z_api_advanced_pubsub_test zenohpico::lib)
      target_link_libraries(z_api_cancellation_test zenohpico::lib)
      target_link_libraries(z_api_local_queryable_test zenohpico::lib)
      target_link_libraries(z_api_local_subscriber_test zenohpico::lib)
      target_link_libraries(z_api_admin_space_test zenohpico::lib)
      target_link_libraries(z_multi_pubsub_test zenohpico::lib)
      target_link_libraries(z_multi_queryable_test zenohpico::lib)
      target_link_libraries(z_api_callback_drop_on_undeclare_test zenohpico::lib)
      target_link_libraries(z_api_batching_test zenohpico::lib)

      configure_file(${PROJECT_SOURCE_DIR}/tests/routed.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/routed.sh COPYONLY)
      configure_file(${PROJECT_SOURCE_DIR}/tests/tls_verify.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tls_verify.sh COPYONLY)
      configure_file(${PROJECT_SOURCE_DIR}/tests/api.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh COPYONLY)

      enable_testing()
      if(Z_FEATURE_LINK_TLS)
        add_test(z_client_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/routed.sh z_client_test --enable-tls)
        add_test(z_tls_verify_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tls_verify.sh z_client_test)
      else()
        add_test(z_client_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/routed.sh z_client_test)
      endif()
      add_test(z_api_alignment_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_alignment_test)
      add_test(z_wildcard_subscription_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_wildcard_subscription_test)
      add_test(z_api_liveliness_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_liveliness_test)
      add_test(z_api_cancellation_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_cancellation_test)
      add_test(z_api_matching_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_matching_test)
      add_test(z_api_source_info_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_source_info_test)
      add_test(z_api_connectivity_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_connectivity_test)
      add_test(z_api_queryable_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_queryable_test)
      add_test(z_api_advanced_pubsub_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_advanced_pubsub_test)
      add_test(z_api_local_queryable_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_local_queryable_test)
      add_test(z_api_local_subscriber_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_local_subscriber_test)
      add_test(z_api_admin_space_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_admin_space_test)
      add_test(z_multi_pubsub_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_multi_pubsub_test)
      add_test(z_multi_queryable_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_multi_queryable_test)
      add_test(z_api_callback_drop_on_undeclare_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_callback_drop_on_undeclare_test)
      add_test(z_api_batching_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_batching_test)
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
          set(CPACK_PACKAGE_VERSION "${SEM_VER}+dev-1")
      elseif(PROJECT_VERSION_TWEAK GREATER 0)
          set(CPACK_PACKAGE_VERSION "${SEM_VER}+pre.${PROJECT_VERSION_TWEAK}-1")
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
