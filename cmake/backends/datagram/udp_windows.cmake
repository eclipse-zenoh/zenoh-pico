zp_register_udp_unicast_backend(NAME udp_windows SYMBOL _z_udp_windows_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_windows.c"
                             SOCKET_COMPONENT windows
                             COMPATIBLE_SYSTEM_LAYERS windows)
