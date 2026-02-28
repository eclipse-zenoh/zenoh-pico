#
# Copyright (c) 2023 ZettaScale Technology
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
#   hyojun.an, <anhj2473@ivis.ai>

cmake_minimum_required(VERSION 3.20)
project(zenohpico_freertos_lwip_examples)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_STANDARD 11)

include(../../cmake/helpers.cmake)
set_default_build_type(Release)

include(FetchContent)

FetchContent_Declare(freertos_kernel
  GIT_REPOSITORY "https://github.com/FreeRTOS/FreeRTOS-Kernel.git"
  GIT_TAG "V11.1.0"
)

FetchContent_Declare(lwip
  GIT_REPOSITORY "https://github.com/lwip-tcpip/lwip.git"
  GIT_TAG "STABLE-2_2_0_RELEASE"
)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE include)
target_compile_options(freertos_config INTERFACE -Wno-error)

set(FREERTOS_HEAP "3" CACHE STRING "" FORCE)
set(FREERTOS_PORT "GCC_POSIX" CACHE STRING "" FORCE)

FetchContent_MakeAvailable(freertos_kernel)

# Configure lwIP
FetchContent_GetProperties(lwip)
if(NOT lwip_POPULATED)
  FetchContent_Populate(lwip)
endif()

add_library(lwip STATIC
  ${lwip_SOURCE_DIR}/src/core/init.c
  ${lwip_SOURCE_DIR}/src/core/def.c
  ${lwip_SOURCE_DIR}/src/core/dns.c
  ${lwip_SOURCE_DIR}/src/core/inet_chksum.c
  ${lwip_SOURCE_DIR}/src/core/ip.c
  ${lwip_SOURCE_DIR}/src/core/mem.c
  ${lwip_SOURCE_DIR}/src/core/memp.c
  ${lwip_SOURCE_DIR}/src/core/netif.c
  ${lwip_SOURCE_DIR}/src/core/pbuf.c
  ${lwip_SOURCE_DIR}/src/core/raw.c
  ${lwip_SOURCE_DIR}/src/core/stats.c
  ${lwip_SOURCE_DIR}/src/core/sys.c
  ${lwip_SOURCE_DIR}/src/core/altcp.c
  ${lwip_SOURCE_DIR}/src/core/altcp_alloc.c
  ${lwip_SOURCE_DIR}/src/core/altcp_tcp.c
  ${lwip_SOURCE_DIR}/src/core/tcp.c
  ${lwip_SOURCE_DIR}/src/core/tcp_in.c
  ${lwip_SOURCE_DIR}/src/core/tcp_out.c
  ${lwip_SOURCE_DIR}/src/core/timeouts.c
  ${lwip_SOURCE_DIR}/src/core/udp.c
  ${lwip_SOURCE_DIR}/src/core/ipv4/autoip.c
  ${lwip_SOURCE_DIR}/src/core/ipv4/dhcp.c
  ${lwip_SOURCE_DIR}/src/core/ipv4/etharp.c
  ${lwip_SOURCE_DIR}/src/core/ipv4/icmp.c
  ${lwip_SOURCE_DIR}/src/core/ipv4/igmp.c
  ${lwip_SOURCE_DIR}/src/core/ipv4/ip4_frag.c
  ${lwip_SOURCE_DIR}/src/core/ipv4/ip4.c
  ${lwip_SOURCE_DIR}/src/core/ipv4/ip4_addr.c
  ${lwip_SOURCE_DIR}/src/core/ipv4/acd.c
  ${lwip_SOURCE_DIR}/src/api/api_lib.c
  ${lwip_SOURCE_DIR}/src/api/api_msg.c
  ${lwip_SOURCE_DIR}/src/api/err.c
  ${lwip_SOURCE_DIR}/src/api/if_api.c
  ${lwip_SOURCE_DIR}/src/api/netbuf.c
  ${lwip_SOURCE_DIR}/src/api/netdb.c
  ${lwip_SOURCE_DIR}/src/api/netifapi.c
  ${lwip_SOURCE_DIR}/src/api/sockets.c
  ${lwip_SOURCE_DIR}/src/api/tcpip.c
  ${lwip_SOURCE_DIR}/src/netif/ethernet.c
)

target_include_directories(lwip PUBLIC
  ${lwip_SOURCE_DIR}/src/include
  ${CMAKE_CURRENT_SOURCE_DIR}
  include
)

target_link_libraries(lwip PUBLIC freertos_kernel)
target_compile_options(lwip PRIVATE -Wno-error)

set(BUILD_SHARED_LIBS OFF)
set(WITH_FREERTOS_LWIP ON)
set(ZENOH_LOG DEBUG)
configure_include_project(ZENOHPICO zenohpico zenohpico::lib "../.." zenohpico "https://github.com/eclipse-zenoh/zenoh-pico" "")

target_link_libraries(zenohpico_static
  PRIVATE
    freertos_kernel
    lwip
)

add_library(main OBJECT main.c arch/sys_arch.c)
target_link_libraries(main
  freertos_kernel
  lwip
)

function(add_example name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name}
    main
    freertos_kernel
    lwip
    zenohpico::lib
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
