set(ZP_PLATFORM_SYSTEM_LAYER flipper)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_FLIPPER)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/flipper/system.c"
    "${PROJECT_SOURCE_DIR}/src/system/flipper/network.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/serial/uart_flipper.c")
set(CHECK_THREADS OFF)
