zp_register_tcp_backend(NAME tcp_windows SYMBOL _z_tcp_windows_stream_ops
                           SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/stream/tcp_windows.c"
                           SOCKET_COMPONENT windows
                           COMPATIBLE_SYSTEM_LAYERS windows)
