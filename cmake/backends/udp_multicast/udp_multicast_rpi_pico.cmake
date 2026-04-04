zp_register_udp_multicast_backend(
  NAME udp_multicast_rpi_pico
  SYMBOL _z_udp_multicast_rpi_pico_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_rpi_pico.c"
          "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_lwip_common.c"
  SOCKET_COMPONENT lwip
  COMPATIBLE_SYSTEM_LAYERS rpi_pico
)
