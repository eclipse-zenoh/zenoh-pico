set(ZP_PLATFORM_SYSTEM_LAYER freertos_lwip)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_FREERTOS_LWIP)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/freertos/system.c"
    "${PROJECT_SOURCE_DIR}/src/system/socket/lwip.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_lwip.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_lwip.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip.c"
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip_common.c")
endif()
