set(ZP_PLATFORM_SYSTEM_LAYER arduino_esp32)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_ARDUINO_ESP32)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/arduino/esp32/system.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/bt/bt_arduino_esp32.cpp"
    "${PROJECT_SOURCE_DIR}/src/system/socket/esp32.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_esp32.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_esp32.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/serial/uart_arduino_esp32.cpp")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_esp32.c")
endif()
set(CHECK_THREADS OFF)
