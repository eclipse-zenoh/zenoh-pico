zp_register_socket_component(
  NAME freertos_plus_tcp
  SOCKET_OPS_SYMBOL _z_freertos_plus_tcp_socket_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/freertos/freertos_plus_tcp/network.c"
)
