zp_register_system_layer(
  NAME macos
  COMPILE_DEFINITIONS ZENOH_MACOS
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/unix/system.c"
)
