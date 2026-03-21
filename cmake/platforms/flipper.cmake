zp_platform_add_definition(ZENOH_FLIPPER)
zp_platform_add_sources("${PROJECT_SOURCE_DIR}/src/system/flipper/system.c"
                        "${PROJECT_SOURCE_DIR}/src/system/flipper/network.c")
set(CHECK_THREADS OFF)
zp_platform_set_rawio_backend(uart_flipper)
