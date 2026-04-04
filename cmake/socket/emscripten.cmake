zp_register_socket_component(
  NAME emscripten
  SOCKET_OPS_SYMBOL _z_emscripten_socket_ops
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/emscripten/network.c"
)
