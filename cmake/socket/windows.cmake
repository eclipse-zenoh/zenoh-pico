zp_register_socket_component(
  NAME windows
  SOCKET_OPS_SYMBOL _z_windows_socket_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/windows/network.c"
  LINK_LIBRARIES Ws2_32 Iphlpapi
)
