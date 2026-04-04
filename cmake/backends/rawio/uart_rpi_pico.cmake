zp_register_serial_backend(NAME uart_rpi_pico SYMBOL _z_uart_rpi_pico_rawio_ops
                          SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/rawio/uart_rpi_pico.c"
                          COMPATIBLE_SYSTEM_LAYERS rpi_pico)
