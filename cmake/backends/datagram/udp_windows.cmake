zp_register_datagram_backend(NAME udp_windows SYMBOL _z_udp_windows_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_windows.c"
                                     "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_windows.c")
