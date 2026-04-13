set(ZP_TRANSPORT_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_zephyr.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_TRANSPORT_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_zephyr.c")
endif()
set(ZP_TRANSPORT_COMPATIBLE_SYSTEM_LAYERS "zephyr")
