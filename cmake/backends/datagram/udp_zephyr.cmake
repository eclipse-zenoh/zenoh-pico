zp_register_udp_unicast_backend(NAME udp_zephyr SYMBOL _z_udp_zephyr_datagram_ops
                             SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_zephyr.c"
                             SOCKET_COMPONENT zephyr
                             COMPATIBLE_SYSTEM_LAYERS zephyr)
