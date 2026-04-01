zp_register_system_layer(
  NAME arduino_opencr
  COMPILE_DEFINITIONS ZENOH_ARDUINO_OPENCR
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/arduino/opencr/system.c"
)
