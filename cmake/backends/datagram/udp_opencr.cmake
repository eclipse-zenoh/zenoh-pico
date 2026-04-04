zp_register_udp_unicast_backend(NAME udp_opencr SYMBOL _z_udp_opencr_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_opencr.cpp"
                             SOCKET_COMPONENT opencr
                             COMPATIBLE_SYSTEM_LAYERS arduino_opencr)
