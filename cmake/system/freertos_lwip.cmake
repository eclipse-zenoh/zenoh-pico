zp_register_system_layer(
  NAME freertos_lwip
  COMPILE_DEFINITIONS ZENOH_FREERTOS_LWIP
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/freertos/system.c"
)
