zp_register_serial_backend(NAME uart_threadx_stm32 SYMBOL _z_uart_threadx_stm32_rawio_ops
                          SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/rawio/uart_threadx_stm32.c"
                          COMPATIBLE_SYSTEM_LAYERS threadx_stm32)
