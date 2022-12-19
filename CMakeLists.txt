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
cmake_minimum_required(VERSION 3.8)

project(libzenohpico VERSION 0.7.0.1 LANGUAGES C)

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

# while in development, use timestamp for patch version:
string(TIMESTAMP PROJECT_VERSION_PATCH "%Y%m%ddev")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# Use cmake .. -DCMAKE_BUILD_TYPE=DEBUG for debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
message(STATUS "Building in ${CMAKE_BUILD_TYPE} mode")

option(BUILD_SHARED_LIBS "Build shared libraries if ON, otherwise build static libraries" ON)
message(STATUS "Build shared library: ${BUILD_SHARED_LIBS}")

option(PACKAGING "Use option on Linux to produce Debian and RPM packages." OFF)
message(STATUS "Produce Debian and RPM packages: ${PACKAGING}")

option(BUILD_EXAMPLES "Use this to also build the examples." ON)
message(STATUS "Build examples: ${BUILD_EXAMPLES}")

option(BUILD_TOOLS "Use this to also build the tools." ON)
message(STATUS "Build examples: ${BUILD_TOOLS}")

option(BUILD_TESTING "Use this to also build tests." ON)
message(STATUS "Build tests: ${BUILD_TESTING}")

option(BUILD_INTEGRATION "Use this to also build integration tests." OFF)
message(STATUS "Build integration: ${BUILD_INTEGRATION}")

option(ZENOH_DEBUG "Use this to set the ZENOH_DEBUG variable." 0)
message(STATUS "Zenoh Level Log: ${ZENOH_DEBUG}")

message(STATUS "Configuring for ${CMAKE_SYSTEM_NAME}")

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_definitions(-DZENOH_LINUX)
  set(JNI_PLATFORM_NAME "linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "BSD")
  add_definitions(-DZENOH_BSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  add_definitions(-DZENOH_MACOS)
  set(MACOSX_RPATH "ON")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  message(FATAL_ERROR "zenoh-pico is not yet available on Windows platform")
  add_definitions(-DZENOH_WIN)
  return()
else()
  message(FATAL_ERROR "zenoh-pico is not yet available on ${CMAKE_SYSTEM_NAME} platform")
  return()
endif()

add_definitions(-DZENOH_DEBUG=${ZENOH_DEBUG})
add_definitions(-DZENOH_C_STANDARD=${CMAKE_C_STANDARD})

if(SKBUILD)
  set(INSTALL_RPATH "zenoh")
  set(INSTALL_NAME_DIR "zenoh")
  set(INSTALL_INCLUDE_NAME_DIR "zenoh/include")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
  add_compile_options(-c -Wall -Wextra -Werror -Wunused -Wstrict-prototypes -pipe -g -O0)
elseif(CMAKE_BUILD_TYPE MATCHES "RELEASE")
  add_compile_options(-pipe -O3)
endif()

set(Libname "zenohpico")

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
include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

if(UNIX)
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
    "src/system/unix/*.c"
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
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
link_directories(${LIBRARY_OUTPUT_PATH})

add_library(${Libname} ${Sources})

target_link_libraries(${Libname} Threads::Threads)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_libraries(${Libname} rt)
endif()

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

if(BUILD_EXAMPLES)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

  if(CMAKE_C_STANDARD MATCHES "99")
    add_executable(z_put ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_put.c)
    add_executable(z_pub ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_pub.c)
    add_executable(z_pub_st ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_pub_st.c)
    add_executable(z_sub ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_sub.c)
    add_executable(z_sub_st ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_sub_st.c)
    add_executable(z_pull ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_pull.c)
    add_executable(z_get ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_get.c)
    add_executable(z_queryable ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_queryable.c)
    add_executable(z_info ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_info.c)
    add_executable(z_scout ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_scout.c)
  else()
    add_executable(z_put ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_put.c)
    add_executable(z_pub ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_pub.c)
    add_executable(z_pub_st ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_pub_st.c)
    add_executable(z_sub ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_sub.c)
    add_executable(z_sub_st ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_sub_st.c)
    add_executable(z_pull ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_pull.c)
    add_executable(z_get ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_get.c)
    add_executable(z_queryable ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_queryable.c)
    add_executable(z_info ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_info.c)
    add_executable(z_scout ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_scout.c)
  endif()

  target_link_libraries(z_put ${Libname})
  target_link_libraries(z_pub ${Libname})
  target_link_libraries(z_pub_st ${Libname})
  target_link_libraries(z_sub ${Libname})
  target_link_libraries(z_sub_st ${Libname})
  target_link_libraries(z_pull ${Libname})
  target_link_libraries(z_get ${Libname})
  target_link_libraries(z_queryable ${Libname})
  target_link_libraries(z_info ${Libname})
  target_link_libraries(z_scout ${Libname})
endif()

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

  target_link_libraries(z_data_struct_test ${Libname})
  target_link_libraries(z_endpoint_test ${Libname})
  target_link_libraries(z_iobuf_test ${Libname})
  target_link_libraries(z_msgcodec_test ${Libname})
  target_link_libraries(z_keyexpr_test ${Libname})

  enable_testing()
  add_test(z_data_struct_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_data_struct_test)
  add_test(z_endpoint_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_endpoint_test)
  add_test(z_iobuf_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_iobuf_test)
  add_test(z_msgcodec_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_msgcodec_test)
  add_test(z_keyexpr_test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/z_keyexpr_test)
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
    target_link_libraries(z_client_test ${Libname})
    target_link_libraries(z_api_alignment_test ${Libname})

    configure_file(${PROJECT_SOURCE_DIR}/tests/routed.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/routed.sh COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/tests/api.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh COPYONLY)

    enable_testing()
    add_test(z_client_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/routed.sh z_client_test)
    add_test(z_api_alignment_test bash ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/api.sh z_api_alignment_test)
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
