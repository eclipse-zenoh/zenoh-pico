zp_register_udp_unicast_backend(NAME udp_freertos_plus_tcp SYMBOL _z_udp_freertos_plus_tcp_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_freertos_plus_tcp.c"
                             SOCKET_COMPONENT freertos_plus_tcp
                             COMPATIBLE_SYSTEM_LAYERS freertos_plus_tcp)
