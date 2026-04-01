set(ZP_BACKEND_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_opencr.cpp")
set(ZP_UDP_BACKEND_UNICAST_SYMBOL "_z_udp_opencr_unicast_ops")
set(ZP_UDP_BACKEND_MULTICAST_SYMBOL "")
zp_udp_multicast_enabled(_zp_udp_multicast_enabled)
if(_zp_udp_multicast_enabled)
  list(APPEND ZP_BACKEND_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_opencr.cpp")
  set(ZP_UDP_BACKEND_MULTICAST_SYMBOL "_z_udp_multicast_opencr_ops")
endif()
set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "arduino_opencr")
set(ZP_BACKEND_SOCKET_COMPONENT "opencr")
