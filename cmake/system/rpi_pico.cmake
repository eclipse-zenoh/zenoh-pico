zp_register_system_layer(
  NAME rpi_pico
  COMPILE_DEFINITIONS ZENOH_RPI_PICO
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/rpi_pico/system.c"
)
