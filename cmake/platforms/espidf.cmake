set(ZP_PLATFORM_SYSTEM_LAYER espidf)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_ESPIDF)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/espidf/system.c"
    "${PROJECT_SOURCE_DIR}/src/system/socket/esp32.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_esp32.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_esp32.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/serial/uart_espidf.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_esp32.c")
endif()
set(CHECK_THREADS OFF)
