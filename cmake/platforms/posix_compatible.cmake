set(ZP_PLATFORM_SYSTEM_LAYER posix_compatible)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_LINUX)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/unix/system.c"
    "${PROJECT_SOURCE_DIR}/src/system/unix/network.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/raweth_unix.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_posix.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_posix.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/serial/tty_posix.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_posix.c")
endif()
set(CHECK_THREADS OFF)
