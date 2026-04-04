zp_register_socket_component(
  NAME opencr
  SOCKET_OPS_SYMBOL _z_arduino_opencr_socket_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/arduino/opencr/network.cpp"
)
