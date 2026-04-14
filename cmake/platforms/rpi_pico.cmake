set(ZP_PLATFORM_SYSTEM_LAYER rpi_pico)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_RPI_PICO)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/rpi_pico/system.c"
    "${PROJECT_SOURCE_DIR}/src/system/rpi_pico/usb_uart.c"
    "${PROJECT_SOURCE_DIR}/src/system/socket/lwip.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_lwip.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_lwip.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/serial/uart_rpi_pico.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_rpi_pico.c"
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip_common.c")
endif()
set(CHECK_THREADS OFF)
