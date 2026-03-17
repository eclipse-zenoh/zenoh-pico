zp_platform_add_definition(ZENOH_ARDUINO_ESP32)
zp_platform_add_sources("${PROJECT_SOURCE_DIR}/src/system/arduino/esp32/system.c"
                        "${PROJECT_SOURCE_DIR}/src/system/arduino/esp32/network.cpp"
                        "${PROJECT_SOURCE_DIR}/src/link/backend/stream/bt_arduino_esp32.cpp")
set(CHECK_THREADS OFF)
zp_platform_set_stream_backend(tcp_esp32)
zp_platform_set_datagram_backend(udp_esp32)
zp_platform_set_rawio_backend(uart_arduino_esp32)
