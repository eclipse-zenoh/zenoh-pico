set(ZP_PLATFORM_SYSTEM_LAYER threadx_stm32)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_THREADX_STM32)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/threadx/stm32/system.c"
    "${PROJECT_SOURCE_DIR}/src/system/threadx/stm32/network.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/serial/uart_threadx_stm32.c")
set(CHECK_THREADS OFF)
