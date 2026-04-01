zp_register_system_layer(
  NAME emscripten
  COMPILE_DEFINITIONS ZENOH_EMSCRIPTEN
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/emscripten/system.c"
)
