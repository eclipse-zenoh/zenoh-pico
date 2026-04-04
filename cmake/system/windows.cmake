zp_register_system_layer(
  NAME windows
  COMPILE_DEFINITIONS ZENOH_WINDOWS _CRT_SECURE_NO_WARNINGS
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/windows/system.c"
)
