zp_register_system_layer(
  NAME threadx_stm32
  COMPILE_DEFINITIONS ZENOH_THREADX_STM32
  SOURCES "${PROJECT_SOURCE_DIR}/src/system/threadx/stm32/system.c"
)
