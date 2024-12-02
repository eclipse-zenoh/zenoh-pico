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

add_executable(zenohpico_rpi_pico_w_examples main.c app_main.c)

#target_compile_definitions(zenohpico_rpi_pico_w_examples PRIVATE PICO_STDIO_DEBUG=1)
pico_enable_stdio_usb(zenohpico_rpi_pico_w_examples 1)
pico_enable_stdio_uart(zenohpico_rpi_pico_w_examples 1)

# Include Zenoh Pico library
include(../../cmake/helpers.cmake)
set_default_build_type(Release)

set(BUILD_SHARED_LIBS OFF)
set(WITH_RPI_PICO_W ON)
set(ZENOH_DEBUG 5)
configure_include_project(ZENOHPICO zenohpico zenohpico::lib "../.." zenohpico "https://github.com/eclipse-zenoh/zenoh-pico" "")

target_link_libraries(zenohpico_static
	pico_cyw43_arch_lwip_sys_freertos
	pico_stdlib
	FreeRTOS-Kernel-Heap4
)

# Configure build  
target_include_directories(zenohpico_static PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/..
)

target_compile_definitions(zenohpico_rpi_pico_w_examples PRIVATE
	WIFI_SSID=\"${WIFI_SSID}\"
	WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
	ZENOH_CONFIG_MODE=\"${ZENOH_CONFIG_MODE}\"
	ZENOH_CONFIG_CONNECT=\"${ZENOH_CONFIG_CONNECT}\"
	ZENOH_CONFIG_LISTEN=\"${ZENOH_CONFIG_LISTEN}\"
)
target_include_directories(zenohpico_rpi_pico_w_examples PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/..
)
target_link_libraries(zenohpico_rpi_pico_w_examples
	zenohpico_static
	pico_cyw43_arch_lwip_sys_freertos
	pico_stdlib
	FreeRTOS-Kernel-Heap4
)

pico_add_extra_outputs(zenohpico_rpi_pico_w_examples)

