set(ZP_BACKEND_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_zephyr.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_BACKEND_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_zephyr.c")
endif()
set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "zephyr")
set(ZP_BACKEND_NETWORK "zephyr")
