set(ZP_PLATFORM_SYSTEM_LAYER freertos_lwip)
set(ZP_PLATFORM_SYSTEM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/freertos/system.c")
set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS ZENOH_FREERTOS_LWIP)
set(ZP_PLATFORM_NETWORK_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/socket/lwip.c")
set(ZP_PLATFORM_TCP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/tcp/tcp_lwip.c")
set(ZP_PLATFORM_UDP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_lwip.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_UDP_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_lwip.c"
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_lwip_common.c")
endif()
