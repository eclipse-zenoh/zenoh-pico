zp_register_udp_multicast_backend(
  NAME udp_multicast_mbed
  SYMBOL _z_udp_multicast_mbed_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/datagram/udp_multicast_mbed.cpp"
  SOCKET_COMPONENT mbed
  COMPATIBLE_SYSTEM_LAYERS mbed
)
