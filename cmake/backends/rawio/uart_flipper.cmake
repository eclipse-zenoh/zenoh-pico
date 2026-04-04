zp_register_serial_backend(NAME uart_flipper SYMBOL _z_uart_flipper_rawio_ops
                          SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/rawio/uart_flipper.c"
                          COMPATIBLE_SYSTEM_LAYERS flipper)
