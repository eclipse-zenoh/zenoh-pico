zp_platform_add_definition(ZENOH_ARDUINO_OPENCR)
zp_platform_add_sources("${PROJECT_SOURCE_DIR}/src/system/arduino/opencr/system.c"
                        "${PROJECT_SOURCE_DIR}/src/system/arduino/opencr/network.cpp")
set(CHECK_THREADS OFF)
zp_platform_set_stream_backend(tcp_opencr)
zp_platform_set_datagram_backend(udp_opencr)
