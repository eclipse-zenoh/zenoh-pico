set(ZP_PLATFORM_SYSTEM_IMPORTED_TARGET mylinux::system)
set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS ZENOH_MYLINUX)
set(ZP_PLATFORM_SYSTEM_PLATFORM_HEADER "zenoh_mylinux_platform.h")
set(ZP_PLATFORM_NETWORK_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/unix/network.c")
set(ZP_PLATFORM_TCP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/tcp/tcp_posix.c")
set(ZP_PLATFORM_UDP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_posix.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_UDP_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_posix.c")
endif()
set(ZP_PLATFORM_SERIAL_IMPORTED_TARGET mylinux::serial_uart)
