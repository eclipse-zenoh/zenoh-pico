set(ZP_PLATFORM_SYSTEM_LAYER zephyr)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_ZEPHYR)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/zephyr/system.c"
    "${PROJECT_SOURCE_DIR}/src/system/zephyr/network.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_zephyr.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_zephyr.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/serial/uart_zephyr.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_zephyr.c")
endif()
