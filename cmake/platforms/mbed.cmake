set(ZP_PLATFORM_SYSTEM_LAYER mbed)
set(ZP_PLATFORM_SYSTEM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/mbed/system.cpp")
set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS ZENOH_MBED)
set(ZP_PLATFORM_NETWORK_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/mbed/network.cpp")
set(CHECK_THREADS OFF)
set(ZP_PLATFORM_TCP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/tcp/tcp_mbed.cpp")
set(ZP_PLATFORM_UDP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_mbed.cpp")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_UDP_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_mbed.cpp")
endif()
set(ZP_PLATFORM_SERIAL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/serial/uart_mbed.cpp")
