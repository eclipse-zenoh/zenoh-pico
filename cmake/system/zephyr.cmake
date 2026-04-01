zp_register_system_layer(
  NAME zephyr
  COMPILE_DEFINITIONS ZENOH_ZEPHYR
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/zephyr/system.c"
)
