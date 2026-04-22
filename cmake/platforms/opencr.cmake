set(ZP_PLATFORM_SYSTEM_LAYER arduino_opencr)
set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_ARDUINO_OPENCR)
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/arduino/opencr/system.c"
    "${PROJECT_SOURCE_DIR}/src/system/arduino/opencr/network.cpp"
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_opencr.cpp"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_opencr.cpp")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_opencr.cpp")
endif()
set(CHECK_THREADS OFF)
