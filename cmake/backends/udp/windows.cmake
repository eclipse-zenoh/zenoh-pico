set(ZP_BACKEND_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_windows.c")
zp_udp_multicast_enabled(_zp_udp_multicast_enabled)
if(_zp_udp_multicast_enabled)
  list(APPEND ZP_BACKEND_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_windows.c")
endif()
set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "windows")
set(ZP_BACKEND_SOCKET_COMPONENT "windows")
