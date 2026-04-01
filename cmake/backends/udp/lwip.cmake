set(ZP_BACKEND_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_lwip.c")
set(ZP_UDP_BACKEND_UNICAST_SYMBOL "_z_udp_lwip_unicast_ops")
set(ZP_UDP_BACKEND_MULTICAST_SYMBOL "")
zp_udp_multicast_enabled(_zp_udp_multicast_enabled)
if(_zp_udp_multicast_enabled)
  list(APPEND ZP_BACKEND_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_lwip.c"
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_lwip_common.c")
  set(ZP_UDP_BACKEND_MULTICAST_SYMBOL "_z_udp_multicast_lwip_ops")
  set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "freertos_lwip")
endif()
set(ZP_BACKEND_SOCKET_COMPONENT "lwip")
