zp_register_tcp_backend(NAME tcp_mbed SYMBOL _z_tcp_mbed_stream_ops
                           SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/stream/tcp_mbed.cpp"
                           SOCKET_COMPONENT mbed
                           COMPATIBLE_SYSTEM_LAYERS mbed)
