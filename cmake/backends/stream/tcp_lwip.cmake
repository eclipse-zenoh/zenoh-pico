zp_register_tcp_backend(NAME tcp_lwip SYMBOL _z_tcp_lwip_stream_ops
                           SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/stream/tcp_lwip.c"
                           SOCKET_COMPONENT lwip)
