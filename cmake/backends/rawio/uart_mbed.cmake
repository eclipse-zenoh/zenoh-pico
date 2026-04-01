zp_register_serial_backend(NAME uart_mbed SYMBOL _z_uart_mbed_rawio_ops
                          SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/rawio/uart_mbed.cpp"
                          COMPATIBLE_SYSTEM_LAYERS mbed)
