zp_register_udp_multicast_backend(
  NAME udp_multicast_windows
  SYMBOL _z_udp_multicast_windows_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_windows.c"
  SOCKET_COMPONENT windows
  COMPATIBLE_SYSTEM_LAYERS windows
)
