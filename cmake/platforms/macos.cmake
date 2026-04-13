set(ZP_PLATFORM_SYSTEM_LAYER macos)
set(ZP_PLATFORM_SYSTEM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/unix/system.c")
set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS ZENOH_MACOS)
set(ZP_PLATFORM_NETWORK_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/unix/network.c")
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/raweth_unix.c")
set(ZP_PLATFORM_TCP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/tcp/tcp_posix.c")
set(ZP_PLATFORM_UDP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_posix.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_UDP_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_posix.c")
endif()
set(ZP_PLATFORM_SERIAL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/serial/tty_posix.c")
