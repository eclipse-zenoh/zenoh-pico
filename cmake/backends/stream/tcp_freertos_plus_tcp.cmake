zp_register_tcp_backend(NAME tcp_freertos_plus_tcp SYMBOL _z_tcp_freertos_plus_tcp_stream_ops
                           SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/stream/tcp_freertos_plus_tcp.c"
                           SOCKET_COMPONENT freertos_plus_tcp
                           COMPATIBLE_SYSTEM_LAYERS freertos_plus_tcp)
