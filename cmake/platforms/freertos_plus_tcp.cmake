set(ZP_PLATFORM_SYSTEM_LAYER freertos_plus_tcp)
set(ZP_PLATFORM_SYSTEM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/freertos/system.c")
set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS ZENOH_FREERTOS_PLUS_TCP)
set(ZP_PLATFORM_NETWORK freertos_plus_tcp)
set(ZP_PLATFORM_NETWORK_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/freertos/freertos_plus_tcp/network.c")
set(ZP_PLATFORM_TCP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/tcp/tcp_freertos_plus_tcp.c")
set(ZP_PLATFORM_UDP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_freertos_plus_tcp.c")
