zp_register_socket_component(
  NAME posix
  SOCKET_OPS_SYMBOL _z_posix_socket_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/unix/network.c"
)
