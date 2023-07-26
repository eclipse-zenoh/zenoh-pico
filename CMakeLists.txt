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

project(zenohpico VERSION 0.10.0.0 LANGUAGES C)

include(CMakePackageConfigHelpers)

option(BUILD_SHARED_LIBS "Build shared libraries if ON, otherwise build static libraries" ON)
option(ZENOH_DEBUG "Use this to set the ZENOH_DEBUG variable." 0)
option(WITH_ZEPHYR "Build for Zephyr RTOS" OFF)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(BUILD_SHARED_LIBS "OFF")
elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
  if(WITH_ZEPHYR)
    set(BUILD_SHARED_LIBS "OFF")
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

if(NOT CMAKE_C_STANDARD)
  if(c_std_11 IN_LIST CMAKE_C_COMPILE_FEATURES)
    set(CMAKE_C_STANDARD 11)
    message(STATUS "Setting C11 as the C Standard")
  else()
    set(CMAKE_C_STANDARD 99)
    message(STATUS "Setting C99 as the C Standard")
  endif()
endif()
set(CMAKE_C_STANDARD_REQUIRED TRUE)
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
  add_definition(ZENOH_NO_STDATOMIC)
elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
  if(WITH_ZEPHYR)
    add_definition(ZENOH_ZEPHYR)
  endif()
else()
  message(FATAL_ERROR "zenoh-pico is not yet available on ${CMAKE_SYSTEM_NAME} platform")
  return()
endif()

add_definition(ZENOH_DEBUG=${ZENOH_DEBUG})

# Print summary of CMAKE configurations
message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode")
message(STATUS "Build shared library: ${BUILD_SHARED_LIBS}")
message(STATUS "Zenoh Level Log: ${ZENOH_DEBUG}")
message(STATUS "Build for Zephyr RTOS: ${WITH_ZEPHYR}")
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

if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
  if(UNIX)
    add_compile_options(-c -Wall -Wextra -Werror -Wunused -Wstrict-prototypes -pipe -g -O0)
  elseif(MSVC)
    add_compile_options(/W4 /WX /Od)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wmissing-prototypes -pipe -g -O0)
  endif()
elseif(CMAKE_BUILD_TYPE MATCHES "RELEASE")
  if(UNIX)
    add_compile_options(-pipe -O3)
  elseif(MSVC)
  # add_compile_options(/O2)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Generic")
    add_compile_options(-pipe -O3)
  endif()
endif()

file(GLOB PublicHeaders "include/*.h"
  "include/zenoh-pico/*.h"
  "include/zenoh-pico/api/*.h"
  "include/zenoh-pico/collections/*.h"
  "include/zenoh-pico/link/*.h"
  "include/zenoh-pico/link/config/*.h"
  "include/zenoh-pico/protocol/*.h"
  "include/zenoh-pico/session/*.h"
  "include/zenoh-pico/system/*.h"
  "include/zenoh-pico/system/link/*.h"
  "include/zenoh-pico/transport/*.h"
  "include/zenoh-pico/utils/*.h"
)
target_include_directories(${Libname} PUBLIC ${PROJECT_SOURCE_DIR}/include)

file(GLOB Sources "src/*.c"
  "src/api/*.c"
  "src/net/*.c"
  "src/collections/*.c"
  "src/link/*.c"
  "src/link/config/*.c"
  "src/link/unicast/*.c"
  "src/link/multicast/*.c"
  "src/protocol/*.c"
  "src/protocol/keyexpr/*.c"
  "src/session/*.c"
  "src/system/*.c"
  "src/transport/*.c"
  "src/transport/common/*.c"
  "src/transport/unicast/*.c"
  "src/transport/unicast/link/*.c"
  "src/transport/unicast/link/task/*.c"
  "src/transport/multicast/*.c"
  "src/transport/multicast/link/*.c"
  "src/transport/multicast/link/task/*.c"
  "src/utils/*.c"
)

if(WITH_ZEPHYR)
  file (GLOB Sources_Zephyr "src/system/zephyr/*.c")
  list(APPEND Sources ${Sources_Zephyr})
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "Darwin" OR CMAKE_SYSTEM_NAME MATCHES "BSD" OR POSIX_COMPATIBLE)
  file (GLOB Sources_Unix "src/system/unix/*.c")
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

  if(BUILD_TESTING)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

    add_executable(z_data_struct_test ${PROJECT_SOURCE_DIR}/tests/z_data_struct_test.c)
    add_executable(z_endpoint_test ${PROJECT_SOURCE_DIR}/tests/z_endpoint_test.c)
    add_executable(z_iobuf_test ${PROJECT_SOURCE_DIR}/tests/z_iobuf_test.c)
    add_executable(z_msgcodec_test ${PROJECT_SOURCE_DIR}/tests/z_msgcodec_test.c)
    add_executable(z_keyexpr_test ${PROJECT_SOURCE_DIR}/tests/z_keyexpr_test.c)
    add_executable(z_api_null_drop_test ${PROJECT_SOURCE_DIR}/tests/z_api_null_drop_test.c)
    add_executable(z_api_double_drop_test ${PROJECT_SOURCE_DIR}/tests/z_api_double_drop_test.c)

    target_link_libraries(z_data_struct_test ${Libname})
    target_link_libraries(z_endpoint_test ${Libname})
    target_link_libraries(z_iobuf_test ${Libname})
    target_link_libraries(z_msgcodec_test ${Libname})
    target_link_libraries(z_keyexpr_test ${Libname})
    target_link_libraries(z_api_null_drop_test ${Libname})
    target_link_libraries(z_api_double_drop_test ${Libname})

    enable_testing()
    add_test(z_data_struct_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_data_struct_test)
    add_test(z_endpoint_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_endpoint_test)
    add_test(z_iobuf_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_iobuf_test)
    add_test(z_msgcodec_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_msgcodec_test)
    add_test(z_keyexpr_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_keyexpr_test)
    add_test(z_api_null_drop_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_api_null_drop_test)
    add_test(z_api_double_drop_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_api_double_drop_test)
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
      set(DEBARCH ${CMAKE_SYSTEM_PROCESSOR})
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