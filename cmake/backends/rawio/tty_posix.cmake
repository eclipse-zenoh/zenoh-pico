zp_register_serial_backend(NAME tty_posix SYMBOL _z_tty_posix_rawio_ops
                          SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/rawio/tty_posix.c"
                          COMPATIBLE_SYSTEM_LAYERS linux macos bsd posix_compatible)
