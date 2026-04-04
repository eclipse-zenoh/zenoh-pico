zp_register_udp_multicast_backend(
  NAME udp_multicast_opencr
  SYMBOL _z_udp_multicast_opencr_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_opencr.cpp"
  SOCKET_COMPONENT opencr
  COMPATIBLE_SYSTEM_LAYERS arduino_opencr
)
