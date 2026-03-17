zp_register_datagram_backend(NAME udp_opencr SYMBOL _z_udp_opencr_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_opencr.cpp"
                                     "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_opencr.cpp")
