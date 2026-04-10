set(ZP_PLATFORM_SYSTEM_LAYER arduino_esp32)
set(ZP_PLATFORM_SYSTEM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/arduino/esp32/system.c")
set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS ZENOH_ARDUINO_ESP32)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/bt/bt_arduino_esp32.cpp")
set(ZP_PLATFORM_NETWORK esp32)
set(ZP_PLATFORM_NETWORK_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/socket/esp32.c")
set(CHECK_THREADS OFF)
set(ZP_PLATFORM_TCP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/tcp/tcp_esp32.c")
set(ZP_PLATFORM_UDP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_esp32.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_UDP_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_esp32.c")
endif()
set(ZP_PLATFORM_SERIAL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/serial/uart_arduino_esp32.cpp")
