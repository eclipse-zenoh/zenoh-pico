zp_register_datagram_backend(NAME udp_esp32 SYMBOL _z_udp_esp32_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_esp32.c"
                                     "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_esp32.c")
