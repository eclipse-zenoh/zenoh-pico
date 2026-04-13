# Minimal one-file platform descriptor template.
#
# Copy this file to:
#   - cmake/platforms/<name>.cmake for an in-tree platform profile, or
#   - <package-config-dir>/platforms/<name>.cmake for an external package.
#
# Replace the paths and names below with the files or imported targets from
# your platform.
#
# Use *_SOURCE_FILES for sources built as part of zenoh-pico.
# Use *_IMPORTED_TARGET for code exported by an external package.
# For each component, choose one form.

# Minimal required

set(ZP_PLATFORM_SYSTEM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/myplatform/system.c")
set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS
    ZENOH_MYPLATFORM)

set(ZP_PLATFORM_NETWORK_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/myplatform/network.c")

set(ZP_PLATFORM_TCP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_myplatform.c")

set(ZP_PLATFORM_UDP_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_myplatform.c")
if(ZP_UDP_MULTICAST_ENABLED)
  list(APPEND ZP_PLATFORM_UDP_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_myplatform.c")
endif()

# Optional, when the platform provides a BT transport implementation.
# set(ZP_PLATFORM_BT_SOURCE_FILES
#     "${PROJECT_SOURCE_DIR}/src/link/transport/bt/bt_myplatform.c")

set(ZP_PLATFORM_SERIAL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/link/transport/serial/uart_myplatform.c")

# Optional

# Only when the logical system-layer name differs from the profile name.
# set(ZP_PLATFORM_SYSTEM_LAYER arduino_opencr)

# Only when include/zenoh-pico/system/common/platform.h should include a
# custom header instead of a built-in one.
# set(ZP_PLATFORM_SYSTEM_PLATFORM_HEADER "zenoh_myplatform_platform.h")

# list(APPEND ZP_PLATFORM_SOURCE_FILES
#      "${PROJECT_SOURCE_DIR}/src/system/myplatform/platform_extra.c")
# list(APPEND ZP_PLATFORM_INCLUDE_DIRS
#      "${PROJECT_SOURCE_DIR}/src/system/myplatform/include")
# list(APPEND ZP_PLATFORM_COMPILE_DEFINITIONS
#      ZENOH_MYPLATFORM_BOARD)
# list(APPEND ZP_PLATFORM_COMPILE_OPTIONS
#      -Wall)
# list(APPEND ZP_PLATFORM_LINK_LIBRARIES
#      myplatform::sdk)
