zp_register_udp_unicast_backend(NAME udp_rpi_pico SYMBOL _z_udp_lwip_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_lwip.c"
                             SOCKET_COMPONENT lwip
                             COMPATIBLE_SYSTEM_LAYERS rpi_pico)
