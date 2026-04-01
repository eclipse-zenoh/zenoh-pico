zp_register_system_layer(
  NAME linux
  COMPILE_DEFINITIONS ZENOH_LINUX
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/unix/system.c"
)
