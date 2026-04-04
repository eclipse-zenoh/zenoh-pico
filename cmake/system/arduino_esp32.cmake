zp_register_system_layer(
  NAME arduino_esp32
  COMPILE_DEFINITIONS ZENOH_ARDUINO_ESP32
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/arduino/esp32/system.c"
)
