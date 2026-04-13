set(ZP_TRANSPORT_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_esp32.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_TRANSPORT_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_esp32.c")
endif()
