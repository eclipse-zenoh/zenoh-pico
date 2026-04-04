zp_register_tcp_backend(NAME tcp_esp32 SYMBOL _z_tcp_esp32_stream_ops
                           SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/stream/tcp_esp32.c"
                           SOCKET_COMPONENT esp32)
