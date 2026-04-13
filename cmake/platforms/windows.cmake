set(ZP_PLATFORM_SYSTEM_LAYER windows)
set(ZP_PLATFORM_SYSTEM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/windows/system.c")
set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS
    ZENOH_WINDOWS
    _CRT_SECURE_NO_WARNINGS)
set(ZP_PLATFORM_NETWORK_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/windows/network.c")
set(ZP_PLATFORM_LINK_LIBRARIES Ws2_32 Iphlpapi)
set(ZP_PLATFORM_TCP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/tcp/tcp_windows.c")
set(ZP_PLATFORM_UDP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_windows.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_UDP_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_windows.c")
endif()
