#
# Copyright (c) 2023 Fictionlab sp. z o.o.
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
#   Błażej Sowa, <blazej@fictionlab.pl>

cmake_minimum_required(VERSION 3.20)
project(zenohpico_freertos_plus_tcp_examples)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_STANDARD 11)

include(../../cmake/helpers.cmake)
set_default_build_type(Release)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SLIRP REQUIRED slirp)

add_library(slirp INTERFACE)
target_link_libraries(slirp INTERFACE ${SLIRP_LINK_LIBRARIES})
target_include_directories(slirp INTERFACE ${SLIRP_INCLUDE_DIRS})
target_compile_options(slirp INTERFACE -Wno-error)

include(FetchContent)

FetchContent_Declare(freertos_kernel
  GIT_REPOSITORY "https://github.com/FreeRTOS/FreeRTOS-Kernel.git"
  GIT_TAG "V10.6.1"
)

FetchContent_Declare(freertos_plus_tcp
  GIT_REPOSITORY "https://github.com/FreeRTOS/FreeRTOS-Plus-TCP.git"
  GIT_TAG "34148c3"
  GIT_SUBMODULES ""
)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE include)
target_compile_options(freertos_config INTERFACE -Wno-error)

set(FREERTOS_HEAP "3" CACHE STRING "" FORCE)
set(FREERTOS_PORT "GCC_POSIX" CACHE STRING "" FORCE)
set(FREERTOS_PLUS_TCP_NETWORK_IF "LIBSLIRP" CACHE STRING "" FORCE)
set(FREERTOS_PLUS_TCP_BUFFER_ALLOCATION "2" CACHE STRING "" FORCE)

FetchContent_MakeAvailable(freertos_kernel freertos_plus_tcp)

set(BUILD_SHARED_LIBS OFF)
set(WITH_FREERTOS_PLUS_TCP ON)
set(ZENOH_DEBUG 3)
configure_include_project(ZENOHPICO zenohpico zenohpico "../.." zenohpico "https://github.com/eclipse-zenoh/zenoh-pico" "")

target_link_libraries(zenohpico
  freertos_kernel
  freertos_plus_tcp
)
target_compile_definitions(zenohpico
  PUBLIC
    Z_FEATURE_MULTI_THREAD=1
    Z_FEATURE_LINK_TCP=1
    Z_FEATURE_SCOUTING_UDP=1
    Z_FEATURE_LINK_UDP_UNICAST=1
    Z_FEATURE_LINK_UDP_MULTICAST=0
    Z_CONFIG_SOCKET_TIMEOUT=1000
)

add_library(main OBJECT main.c)
target_link_libraries(main
  freertos_kernel
  freertos_plus_tcp
)

function(add_example name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name}
    main
    freertos_kernel
    freertos_plus_tcp
    zenohpico
  )
endfunction()

add_example(z_get)
add_example(z_pub_st)
add_example(z_pub)
add_example(z_pull)
add_example(z_put)
add_example(z_queryable)
add_example(z_scout)
add_example(z_sub_st)
add_example(z_sub)
