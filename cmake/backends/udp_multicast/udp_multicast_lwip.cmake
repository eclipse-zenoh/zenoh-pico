zp_register_udp_multicast_backend(
  NAME udp_multicast_lwip
  SYMBOL _z_udp_multicast_lwip_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_lwip.c"
          "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_lwip_common.c"
  SOCKET_COMPONENT lwip
  COMPATIBLE_SYSTEM_LAYERS freertos_lwip
)
