zp_register_datagram_backend(NAME udp_lwip SYMBOL _z_udp_lwip_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_lwip.c"
                                     "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_lwip.c")
