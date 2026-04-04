zp_register_system_layer(
  NAME espidf
  COMPILE_DEFINITIONS ZENOH_ESPIDF
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/espidf/system.c"
)
