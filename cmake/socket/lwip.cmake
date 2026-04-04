zp_register_socket_component(
  NAME lwip
  SOCKET_OPS_SYMBOL _z_lwip_socket_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/socket/lwip.c"
)
