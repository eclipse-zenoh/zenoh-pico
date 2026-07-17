set(ZP_PLATFORM_SYSTEM_LAYER wasi)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_WASI)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/wasi/system.c"
    "${PROJECT_SOURCE_DIR}/src/system/wasi/network.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_posix.c")
set(CHECK_THREADS OFF)
