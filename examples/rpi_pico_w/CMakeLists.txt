cmake_minimum_required(VERSION 3.13)

# Set options
set(WIFI_SSID "" CACHE STRING "WiFi SSID")
set(WIFI_PASSWORD "" CACHE STRING "WiFi Password")

set(ZENOH_CONFIG_MODE "client" CACHE STRING "ZENOH_CONFIG_MODE")
set(ZENOH_CONFIG_CONNECT CACHE STRING "ZENOH_CONFIG_CONNECT")
set(ZENOH_CONFIG_LISTEN CACHE STRING "ZENOH_CONFIG_LISTEN")

message(STATUS "WIFI_SSID: ${WIFI_SSID}")
if(WIFI_PASSWORD STREQUAL "")
  message(STATUS "WIFI_PASSWORD is empty")
else()
  message(STATUS "WIFI_PASSWORD is set")
endif()

message(STATUS "ZENOH_CONFIG_MODE: ${ZENOH_CONFIG_MODE}")
message(STATUS "ZENOH_CONFIG_CONNECT: ${ZENOH_CONFIG_CONNECT}")
message(STATUS "ZENOH_CONFIG_LISTEN: ${ZENOH_CONFIG_LISTEN}")

set(CMAKE_C_STANDARD 11)

# Include Raspberry Pico SDK
set(PICO_BOARD "pico_w")
if(NOT DEFINED ENV{PICO_SDK_PATH})
  message(FATAL_ERROR "PICO_SDK_PATH environment variable is not set. Please set it to the location of the Pico SDK.")
endif()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Include FreeRTOS SDK
include(FreeRTOS_Kernel_import.cmake)

# Configure project
project(zenohpico_rpi_pico_w_examples C CXX ASM)

add_compile_definitions(LWIP_TIMEVAL_PRIVATE=0)
pico_sdk_init()

# Include Zenoh Pico library
include(../../cmake/helpers.cmake)
set_default_build_type(Release)

set(BUILD_SHARED_LIBS OFF)
set(WITH_RPI_PICO_W ON)
configure_include_project(ZENOHPICO zenohpico zenohpico::lib "../.." zenohpico "https://github.com/eclipse-zenoh/zenoh-pico" "")

target_link_libraries(zenohpico_static
  pico_stdlib
  pico_cyw43_arch_lwip_sys_freertos
  FreeRTOS-Kernel-Heap4
)

# Configure build  
target_include_directories(zenohpico_static PRIVATE 
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
)

add_library(main STATIC main.c)

target_compile_definitions(main PRIVATE
  WIFI_SSID=\"${WIFI_SSID}\"
  WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)
target_include_directories(main PRIVATE 
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(main
  pico_stdlib
  pico_cyw43_arch_lwip_sys_freertos
  FreeRTOS-Kernel-Heap4
)

function(add_example name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name}
    main
    pico_stdlib
    pico_cyw43_arch_lwip_sys_freertos
    FreeRTOS-Kernel-Heap4
    zenohpico::lib
  )
  target_compile_definitions(${name} PRIVATE
    ZENOH_CONFIG_MODE=\"${ZENOH_CONFIG_MODE}\"
    ZENOH_CONFIG_CONNECT=\"${ZENOH_CONFIG_CONNECT}\"
    ZENOH_CONFIG_LISTEN=\"${ZENOH_CONFIG_LISTEN}\"
  )
  target_include_directories(${name} PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
  )
  pico_enable_stdio_usb(${name} 1)
  pico_enable_stdio_uart(${name} 1)
  pico_add_extra_outputs(${name})
endfunction()

add_example(z_get)
add_example(z_pub)
add_example(z_pub_st)
add_example(z_pull)
add_example(z_put)
add_example(z_queryable)
add_example(z_scout)
add_example(z_sub)
add_example(z_sub_st)
