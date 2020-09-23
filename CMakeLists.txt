#
# Copyright (c) 2017, 2020 ADLINK Technology Inc.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
#   ADLINK zenoh team, <zenoh@adlink-labs.tech>
#
cmake_minimum_required(VERSION 3.0)

project(libzenohc VERSION 0.4.2 LANGUAGES C)

# while in development, use timestamp for patch version:
string(TIMESTAMP PROJECT_VERSION_PATCH "%Y%m%ddev")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option (PACKAGING "Use option on Linux to produce Debian and RPM packages." OFF)
option (UNIX_TARGET "Use this for Linux, Macos, etc." ON)
option (CONTIKI "Use this for Contiki Cross compilation." OFF)
option (TESTS "Use this to also build the tests." ON)
option (EXAMPLES "Use this to also build the examples." ON)
option (SWIG_JAVA "Use this to build the zenohc_java library, assuming the Swig generated C files in zenoh-c/src/ ." OFF)

# Configure the transport to use with the following variables
set (ZENOH_TRANSPORT_TCP "ON")
set (ZENOH_TRANSPORT_UDP "OFF")
set (ZENOH_TRANSPORT_BLE "OFF")
set (ZENOH_TRANSPORT_I2C "OFF")

# Use cmake .. -DCMAKE_BUILD_TYPE=Debug for debug / Release

# Configure the debug level
#
# ZENOH_DEBUG :
#   - 2 : ERROR + DEBUG
#   - 1 : ERROR
#   - 0 : NOTHING

if (DEFINED ZENOH_DEBUG)
  set (ZENOH_DEBUG_OPT "-DZENOH_DEBUG=${ZENOH_DEBUG}")
else()
  set(ZENOH_DEBUG_OPT "-DZENOH_DEBUG=1") 
endif() 

message(STATUS "Configuring for ${CMAKE_SYSTEM_NAME}")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(ZENOH_PLATFORM_OPT "-DZENOH_LINUX=1")  
  set(JNI_PLATFORM_NAME "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(ZENOH_PLATFORM_OPT "-DZENOH_MACOS=1")
  set(MACOSX_RPATH "ON")  
  set(JNI_PLATFORM_NAME "darwin")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message(FATAL_ERROR "zenoh-c is not yet available on Windows platform")
  # set(ZENOH_PLATFORM_OPT "-DZENOH_WIN=1")
  # set(JNI_PLATFORM_NAME "win32")
endif()

if (SKBUILD)
  set(INSTALL_RPATH "zenoh")
  set(INSTALL_NAME_DIR "zenoh")
  set(INSTALL_INCLUDE_NAME_DIR "zenoh/include")
endif()

if (ZENOH_TRANSPORT_TCP STREQUAL "ON")
  set (ZENOH_TRANSPORT_OPT "-DZENOH_TRANSPORT_TCP_IP=1")
endif()
if (ZENOH_TRANSPORT_UDP STREQUAL "ON")
  set (ZENOH_TRANSPORT_OPT "${ZENOH_TRANSPORT_OPT} -DZENOH_TRANSPORT_UDP_IP=1")
endif()
if (ZENOH_TRANSPORT_BLE STREQUAL "ON")
  set (ZENOH_TRANSPORT_OPT "${ZENOH_TRANSPORT_OPT} -DZENOH_TRANSPORT_BLE=1")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


# Note: don't use CMAKE_C_STANDARD to be compatible with cmake <3.1.0
set(CMAKE_C_FLAGS_DEBUG "${ZENOH_DEBUG_OPT} ${ZENOH_PLATFORM_OPT} -std=gnu99 -Wall -Wextra -Werror -Wpedantic -Wunused -pipe -g -O0")
set(CMAKE_C_FLAGS_GCOV "${ZENOH_DEBUG_OPT} ${ZENOH_PLATFORM_OPT} -std=gnu99 -fprofile-arcs -ftest-coverage -Wall -Wextra -Werror -Wpedantic -Wunused -pipe -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${ZENOH_DEBUG_OPT} ${ZENOH_PLATFORM_OPT} -std=gnu99 -DNDEBUG -pipe -O3")


if (NOT SWIG_JAVA)
  set(Libname "zenohc")
  file(GLOB PublicHeaders "include/*.h" "include/zenoh/*.h" "include/zenoh/net/*.h")
  include_directories(
      "${PROJECT_SOURCE_DIR}/include"
      "${PROJECT_SOURCE_DIR}/include/net"
    )
else()
  set(Libname "zenohc_java")

  if (NOT JNI_INCLUDE_HOME)
    message(STATUS "Looking for local JDK with JNI")
    find_package(JNI REQUIRED)
  else()
    message(STATUS "Using JNI headers from ${JNI_INCLUDE_HOME}")
    get_filename_component(JNI_INCL_PATH1 "${JNI_INCLUDE_HOME}" ABSOLUTE)
    get_filename_component(JNI_INCL_PATH2 "${JNI_INCLUDE_HOME}/${JNI_PLATFORM_NAME}" ABSOLUTE)
    set(JNI_INCLUDE_DIRS "${JNI_INCL_PATH1};${JNI_INCL_PATH2}")
  endif()
  message(STATUS "Using JNI include paths: ${JNI_INCLUDE_DIRS}")

  include_directories(
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/net"
    "${JNI_INCLUDE_DIRS}"
  )
endif()


if (UNIX_TARGET)
  file(GLOB Sources "src/*.c" "src/net/*.c" "src/net/unix/*.c")
endif()

link_directories("${PROJECT_SOURCE_DIR}/build")

add_library(${Libname} SHARED ${Sources})

if (UNIX_TARGET)
  target_link_libraries(${Libname} pthread)
endif()

target_link_libraries(${Libname})

if (NOT SWIG_JAVA)
  install(TARGETS ${Libname} LIBRARY
            DESTINATION lib
            COMPONENT Library
          )
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh.h
            DESTINATION include
            COMPONENT Headers
          )
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh
            DESTINATION include
            COMPONENT Headers
          )
endif()


if (EXAMPLES)
  add_executable(zn_write example/net/zn_write.c)
  add_executable(zn_stream example/net/zn_stream.c)
  add_executable(zn_sub example/net/zn_sub.c)
  add_executable(zn_pull example/net/zn_pull.c)
  add_executable(zn_query example/net/zn_query.c)
  add_executable(zn_storage example/net/zn_storage.c)
  add_executable(zn_eval example/net/zn_eval.c)
  add_executable(zn_info example/net/zn_info.c)
  add_executable(zn_pub_thr example/net/zn_pub_thr.c)
  add_executable(zn_sub_thr example/net/zn_sub_thr.c)
  add_executable(zn_scout example/net/zn_scout.c)

  target_link_libraries(zn_write ${Libname})
  target_link_libraries(zn_stream ${Libname})
  target_link_libraries(zn_sub ${Libname})
  target_link_libraries(zn_pull ${Libname})
  target_link_libraries(zn_query ${Libname})
  target_link_libraries(zn_storage ${Libname})
  target_link_libraries(zn_eval ${Libname})
  target_link_libraries(zn_info ${Libname})
  target_link_libraries(zn_pub_thr ${Libname})
  target_link_libraries(zn_sub_thr ${Libname})
  target_link_libraries(zn_scout ${Libname})
endif(EXAMPLES)

if (TESTS)
  add_executable(z_data_struct test/z_data_struct.c)
  add_executable(z_mvar_test test/z_mvar_test.c)
  add_executable(z_rname_test test/z_rname_test.c)
  add_executable(zn_client_test test/zn_client_test.c)
  add_executable(zn_large_data_test test/zn_large_data_test.c)

  target_link_libraries(z_data_struct ${Libname})
  target_link_libraries(z_mvar_test ${Libname})
  target_link_libraries(z_rname_test ${Libname})
  target_link_libraries(zn_client_test ${Libname})
  target_link_libraries(zn_large_data_test ${Libname})

  configure_file(test/routed.sh routed.sh COPYONLY)

  enable_testing()
  add_test(z_data_struct z_data_struct)
  add_test(z_mvar_test z_mvar_test)
  add_test(z_rname_test z_rname_test)
  add_test(zn_client_test bash routed.sh zn_client_test)
  add_test(zn_large_data_test bash routed.sh zn_large_data_test)
endif(TESTS)




# For packaging
if (PACKAGING)

  # If not specified, try to guess target architecture, trying dpkg then uname
  if(NOT TARGET_ARCH)
    execute_process(COMMAND dpkg-architecture -qDEB_TARGET_ARCH OUTPUT_VARIABLE TARGET_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
  if(NOT TARGET_ARCH)
    execute_process(COMMAND uname -i OUTPUT_VARIABLE TARGET_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  set(CPACK_COMPONENTS_ALL Library Headers)
  set(CPACK_COMPONENT_LIBRARY_GROUP "lib")
  set(CPACK_COMPONENT_HEADERS_GROUP "dev")
  set(CPACK_COMPONENT_HEADERS_DEPENDS Library)

  set(CPACK_PACKAGE_CHECKSUM MD5)
  set(CPACK_PACKAGE_VENDOR "The Eclipse Foundation")
  set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
  set(CPACK_COMPONENT_LIB_DESCRIPTION "The C client library for Eclipse zenoh")
  set(CPACK_COMPONENT_DEV_DESCRIPTION "${CPACK_COMPONENT_LIB_DESCRIPTION} - devel files")

  # Sources package
  set(CPACK_SOURCE_GENERATOR "TGZ")
  set(CPACK_SOURCE_IGNORE_FILES "/.git/;/build/")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-src-${PROJECT_VERSION}")
  set(CPACK_GENERATOR "")

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Configure packaging for Linux ${TARGET_ARCH}")
    set(CPACK_GENERATOR "DEB;RPM")
  
    # DEB package
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ADLINK zenoh team, <zenoh@adlink-labs.tech>")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${TARGET_ARCH})
    set(CPACK_DEB_COMPONENT_INSTALL ON)
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_LIB_PACKAGE_NAME ${PROJECT_NAME})   # avoid "-lib" suffix for "lib" package
    set(CPACK_DEBIAN_LIB_PACKAGE_DEPENDS "libc6 (>=2.12)")
    set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "${CPACK_DEBIAN_LIB_PACKAGE_NAME} (=${PROJECT_VERSION})")

    # RPM package
    execute_process(COMMAND yum install -y rpm-build)  # Because rpm-build not available on manylinux
    set(CPACK_RPM_COMPONENT_INSTALL ON)
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    set(CPACK_RPM_LIB_PACKAGE_NAME ${PROJECT_NAME})   # avoid "-lib" suffix for "lib" package
    set(CPACK_RPM_DEV_PACKAGE_REQUIRES "${CPACK_DEBIAN_LIB_PACKAGE_NAME} = ${PROJECT_VERSION}")
  endif()

  include(CPack)
endif()