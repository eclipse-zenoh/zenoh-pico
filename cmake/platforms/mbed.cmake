zp_platform_add_definition(ZENOH_MBED)
zp_platform_add_sources("${PROJECT_SOURCE_DIR}/src/system/mbed/system.cpp"
                        "${PROJECT_SOURCE_DIR}/src/system/mbed/network.cpp")
set(CHECK_THREADS OFF)
zp_platform_set_stream_backend(tcp_mbed)
zp_platform_set_datagram_backend(udp_mbed)
zp_platform_set_rawio_backend(uart_mbed)
