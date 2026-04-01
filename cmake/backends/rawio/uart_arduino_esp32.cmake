zp_register_serial_backend(NAME uart_arduino_esp32 SYMBOL _z_uart_arduino_esp32_rawio_ops
                          SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/rawio/uart_arduino_esp32.cpp"
                          COMPATIBLE_SYSTEM_LAYERS arduino_esp32)
