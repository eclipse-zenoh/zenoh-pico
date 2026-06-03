set(ZP_PLATFORM_SYSTEM_LAYER mbed)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_MBED)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/mbed/system.cpp"
    "${PROJECT_SOURCE_DIR}/src/system/mbed/network.cpp"
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_mbed.cpp"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_mbed.cpp"
    "${PROJECT_SOURCE_DIR}/src/link/transport/serial/uart_mbed.cpp")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_mbed.cpp")
endif()
set(CHECK_THREADS OFF)
