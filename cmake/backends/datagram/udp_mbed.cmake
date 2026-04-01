zp_register_udp_unicast_backend(NAME udp_mbed SYMBOL _z_udp_mbed_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_mbed.cpp"
                             SOCKET_COMPONENT mbed
                             COMPATIBLE_SYSTEM_LAYERS mbed)
