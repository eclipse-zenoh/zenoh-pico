zp_register_socket_component(
  NAME mbed
  SOCKET_OPS_SYMBOL _z_mbed_socket_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/mbed/network.cpp"
)
