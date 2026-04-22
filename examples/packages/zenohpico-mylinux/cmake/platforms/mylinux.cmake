set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_MYLINUX)
set(ZP_PLATFORM_SYSTEM_PLATFORM_HEADER "zenoh_mylinux_platform.h")
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/unix/network.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_posix.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_posix.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_posix.c")
endif()
list(APPEND ZP_PLATFORM_INCLUDE_DIRS
     "${CMAKE_CURRENT_LIST_DIR}/../../../../include")
list(APPEND ZP_PLATFORM_LINK_LIBRARIES
     mylinux::system
     mylinux::serial_uart)
