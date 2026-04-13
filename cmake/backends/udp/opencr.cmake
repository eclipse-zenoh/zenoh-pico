set(ZP_BACKEND_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_opencr.cpp")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_BACKEND_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_opencr.cpp")
endif()
set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "arduino_opencr")
