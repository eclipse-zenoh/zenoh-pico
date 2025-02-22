cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD "pico_w" CACHE STRING "Raspberry Pi Pico board: pico, pico_w, pico2, pico2_w")

set(WIFI_SUPPORT_ENABLED 0)
if(PICO_BOARD STREQUAL "pico_w" OR PICO_BOARD STREQUAL "pico2_w")
  set(WIFI_SUPPORT_ENABLED 1)
endif()

# Set options
set(WIFI_SSID "" CACHE STRING "WiFi SSID")
set(WIFI_PASSWORD "" CACHE STRING "WiFi Password")

set(ZENOH_CONFIG_MODE "client" CACHE STRING "ZENOH_CONFIG_MODE")
set(ZENOH_CONFIG_CONNECT CACHE STRING "ZENOH_CONFIG_CONNECT")
set(ZENOH_CONFIG_LISTEN CACHE STRING "ZENOH_CONFIG_LISTEN")
option(ZENOH_USB_UART "Enable USB UART" OFF)

message(STATUS "PICO_BOARD: ${PICO_BOARD}")
message(STATUS "WIFI_SSID: ${WIFI_SSID}")
if(WIFI_PASSWORD STREQUAL "")
  message(STATUS "WIFI_PASSWORD is empty")
else()
  message(STATUS "WIFI_PASSWORD is set")
endif()

message(STATUS "ZENOH_CONFIG_MODE: ${ZENOH_CONFIG_MODE}")
message(STATUS "ZENOH_CONFIG_CONNECT: ${ZENOH_CONFIG_CONNECT}")
message(STATUS "ZENOH_CONFIG_LISTEN: ${ZENOH_CONFIG_LISTEN}")
message(STATUS "ZENOH_USB_UART: ${ZENOH_USB_UART}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h"
)

set(CMAKE_C_STANDARD 11)

# Include Raspberry Pi Pico SDK
if(NOT DEFINED ENV{PICO_SDK_PATH})
  message(FATAL_ERROR "PICO_SDK_PATH environment variable is not set. Please set it to the location of the Pico SDK.")
endif()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Include FreeRTOS SDK
include(FreeRTOS_Kernel_import.cmake)

# Configure project
project(zenohpico_rpi_pico_examples C CXX ASM)

if(WIFI_SUPPORT_ENABLED)
  set(WIFI_LIB pico_cyw43_arch_lwip_sys_freertos)
else()
  set(WIFI_LIB "")
endif()

if(ZENOH_USB_UART)
  set(USB_LIBS 
    tinyusb_host
    tinyusb_device
    tinyusb_board
  )
  set(USB_INCLUDE 
    ${CMAKE_CURRENT_LIST_DIR}/include/tusb
  )
else()
  set(USB_LIBS "")
  set(USB_INCLUDE "")
endif()

add_compile_definitions(LWIP_TIMEVAL_PRIVATE=0)
pico_sdk_init()

# Include Zenoh Pico library
include(../../cmake/helpers.cmake)
set_default_build_type(Release)

if (NOT WIFI_SUPPORT_ENABLED)
  declare_cache_var(Z_FEATURE_LINK_TCP 0 STRING "TCP support")
  declare_cache_var(Z_FEATURE_LINK_UDP_MULTICAST 0 STRING "UDP multicast support")
  declare_cache_var(Z_FEATURE_LINK_UDP_UNICAST 0 STRING "UDP unicast support")
  declare_cache_var(Z_FEATURE_SCOUTING_UDP 0 STRING "Scouting support")
endif()

declare_cache_var(Z_FEATURE_LINK_SERIAL 1 STRING "Serial support")
if(ZENOH_USB_UART)
  declare_cache_var(Z_FEATURE_UNSTABLE_API 1 STRING "Enable unstable API")
  declare_cache_var(Z_FEATURE_LINK_SERIAL_USB 1 STRING "Serial USB support")
else()
  declare_cache_var(Z_FEATURE_LINK_SERIAL_USB 0 STRING "Serial USB support")
endif()

set(BUILD_SHARED_LIBS OFF)
set(WITH_RPI_PICO ON)

configure_include_project(ZENOHPICO zenohpico zenohpico::lib "../.." zenohpico "https://github.com/eclipse-zenoh/zenoh-pico" "")

target_link_libraries(zenohpico_static
  hardware_uart 
  pico_stdlib
  pico_rand
  FreeRTOS-Kernel-Heap4
  ${WIFI_LIB} 
  ${USB_LIBS} 
)

# Configure build  
target_include_directories(zenohpico_static PRIVATE 
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${USB_INCLUDE} 
)

add_library(main STATIC main.c)

target_include_directories(main PRIVATE 
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(main
  pico_stdlib
  pico_rand
  FreeRTOS-Kernel-Heap4
  ${WIFI_LIB} 
)

function(add_example name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name}
    main
    hardware_uart 
    pico_stdlib
    pico_rand
    FreeRTOS-Kernel-Heap4
    zenohpico::lib
    ${WIFI_LIB} 
    ${USB_LIBS} 
  )
  target_include_directories(${name} PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${USB_INCLUDE} 
  )
  if(ZENOH_USB_UART)
    pico_enable_stdio_uart(${name} 1)
    pico_enable_stdio_usb(${name} 0)
  else()
    pico_enable_stdio_uart(${name} 0)
    pico_enable_stdio_usb(${name} 1)
  endif()
  pico_add_extra_outputs(${name})
endfunction()

add_example(z_get)
add_example(z_pub)
add_example(z_pub_st)
add_example(z_pub_thr)
add_example(z_pull)
add_example(z_put)
add_example(z_queryable)
add_example(z_scout)
add_example(z_sub)
add_example(z_sub_st)
add_example(z_sub_thr)
