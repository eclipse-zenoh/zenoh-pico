zp_register_socket_component(
  NAME esp32
  SOCKET_OPS_SYMBOL _z_esp32_socket_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/socket/esp32.c"
)
