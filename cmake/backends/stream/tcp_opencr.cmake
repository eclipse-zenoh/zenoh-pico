zp_register_tcp_backend(NAME tcp_opencr SYMBOL _z_tcp_opencr_stream_ops
                           SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/stream/tcp_opencr.cpp"
                           SOCKET_COMPONENT opencr
                           COMPATIBLE_SYSTEM_LAYERS arduino_opencr)
