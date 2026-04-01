zp_register_udp_multicast_backend(
  NAME udp_multicast_esp32
  SYMBOL _z_udp_multicast_esp32_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_esp32.c"
  SOCKET_COMPONENT esp32
)
