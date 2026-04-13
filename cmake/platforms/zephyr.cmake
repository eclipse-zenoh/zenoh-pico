set(ZP_PLATFORM_SYSTEM_LAYER zephyr)
set(ZP_PLATFORM_SYSTEM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/zephyr/system.c")
set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS ZENOH_ZEPHYR)
set(ZP_PLATFORM_NETWORK_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/zephyr/network.c")
set(ZP_PLATFORM_TCP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/tcp/tcp_zephyr.c")
set(ZP_PLATFORM_UDP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_zephyr.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_UDP_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_zephyr.c")
endif()
set(ZP_PLATFORM_SERIAL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/serial/uart_zephyr.c")
