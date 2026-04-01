zp_register_udp_multicast_backend(
  NAME udp_multicast_zephyr
  SYMBOL _z_udp_multicast_zephyr_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_zephyr.c"
  SOCKET_COMPONENT zephyr
  COMPATIBLE_SYSTEM_LAYERS zephyr
)
