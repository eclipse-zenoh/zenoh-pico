set(ZP_BACKEND_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_freertos_plus_tcp.c")
set(ZP_UDP_BACKEND_UNICAST_SYMBOL "_z_udp_freertos_plus_tcp_unicast_ops")
set(ZP_UDP_BACKEND_MULTICAST_SYMBOL "")
set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "freertos_plus_tcp")
set(ZP_BACKEND_SOCKET_COMPONENT "freertos_plus_tcp")
