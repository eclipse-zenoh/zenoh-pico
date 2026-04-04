zp_register_tcp_backend(NAME tcp_zephyr SYMBOL _z_tcp_zephyr_stream_ops
                           SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/stream/tcp_zephyr.c"
                           SOCKET_COMPONENT zephyr
                           COMPATIBLE_SYSTEM_LAYERS zephyr)
