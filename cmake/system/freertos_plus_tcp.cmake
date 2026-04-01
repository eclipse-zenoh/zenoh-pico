zp_register_system_layer(
  NAME freertos_plus_tcp
  COMPILE_DEFINITIONS ZENOH_FREERTOS_PLUS_TCP
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/freertos/system.c"
)
