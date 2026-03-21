zp_register_datagram_backend(NAME udp_posix SYMBOL _z_udp_posix_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_posix.c"
                                     "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_posix.c")
