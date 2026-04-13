set(ZP_TRANSPORT_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_opencr.cpp")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_TRANSPORT_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_opencr.cpp")
endif()
set(ZP_TRANSPORT_COMPATIBLE_SYSTEM_LAYERS "arduino_opencr")
