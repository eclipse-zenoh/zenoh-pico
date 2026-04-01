zp_register_udp_multicast_backend(
  NAME udp_multicast_posix
  SYMBOL _z_udp_multicast_posix_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_posix.c"
  SOCKET_COMPONENT posix
)
