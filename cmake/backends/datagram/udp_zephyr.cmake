zp_register_datagram_backend(NAME udp_zephyr SYMBOL _z_udp_zephyr_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_zephyr.c"
                                     "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_zephyr.c")
