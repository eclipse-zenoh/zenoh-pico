zp_register_serial_backend(NAME uart_zephyr SYMBOL _z_uart_zephyr_rawio_ops
                          SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/rawio/uart_zephyr.c"
                          COMPATIBLE_SYSTEM_LAYERS zephyr)
