zp_register_serial_backend(NAME uart_espidf SYMBOL _z_uart_espidf_rawio_ops
                          SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/rawio/uart_espidf.c"
                          COMPATIBLE_SYSTEM_LAYERS espidf)
