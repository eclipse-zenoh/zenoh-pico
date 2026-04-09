set(ZP_BACKEND_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_opencr.cpp")
zp_udp_multicast_enabled(_zp_udp_multicast_enabled)
if(_zp_udp_multicast_enabled)
  list(APPEND ZP_BACKEND_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_opencr.cpp")
endif()
set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "arduino_opencr")
set(ZP_BACKEND_SOCKET_COMPONENT "opencr")
