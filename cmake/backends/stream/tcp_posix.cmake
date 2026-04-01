zp_register_tcp_backend(NAME tcp_posix SYMBOL _z_tcp_posix_stream_ops
                           SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/stream/tcp_posix.c"
                           SOCKET_COMPONENT posix)
