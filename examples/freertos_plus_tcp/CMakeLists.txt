cmake_minimum_required(VERSION 3.20)
project(zenohpico_freertos_plus_tcp_examples)

set(CMAKE_SYSTEM_NAME "Generic")

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
  GIT_REPOSITORY "https://github.com/bjsowa/FreeRTOS-Plus-TCP.git"
  GIT_TAG "44289aa"
  GIT_SUBMODULES ""
)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM
  INTERFACE
    include
)

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
    Z_MULTI_THREAD=0
    Z_LINK_TCP=1
    Z_SCOUTING_UDP=1
    Z_LINK_UDP_UNICAST=1
    Z_LINK_UDP_MULTICAST=0
)    

add_executable(z_pub
  main.c
  z_pub.c
)

target_link_libraries(z_pub
  freertos_kernel
  freertos_plus_tcp
  zenohpico
)
