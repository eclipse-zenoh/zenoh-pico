set(ZP_BACKEND_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_lwip.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_BACKEND_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_lwip.c"
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_lwip_common.c")
  set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "freertos_lwip")
endif()
