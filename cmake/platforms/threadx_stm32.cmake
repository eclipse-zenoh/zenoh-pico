zp_platform_add_definition(ZENOH_THREADX_STM32)
zp_platform_add_sources("${PROJECT_SOURCE_DIR}/src/system/threadx/stm32/system.c"
                        "${PROJECT_SOURCE_DIR}/src/system/threadx/stm32/network.c")
set(CHECK_THREADS OFF)
zp_platform_set_rawio_backend(uart_threadx_stm32)
