zp_register_socket_component(
  NAME zephyr
  SOCKET_OPS_SYMBOL _z_zephyr_socket_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/zephyr/network.c"
)
