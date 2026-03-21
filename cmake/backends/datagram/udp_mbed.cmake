zp_register_datagram_backend(NAME udp_mbed SYMBOL _z_udp_mbed_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_mbed.cpp"
                                     "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_mbed.cpp")
