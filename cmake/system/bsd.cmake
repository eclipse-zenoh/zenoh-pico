zp_register_system_layer(
  NAME bsd
  COMPILE_DEFINITIONS ZENOH_BSD
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/unix/system.c"
)
