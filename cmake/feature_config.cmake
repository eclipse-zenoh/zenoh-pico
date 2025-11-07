# Zenoh pico feature configuration options
set(FRAG_MAX_SIZE 4096 CACHE STRING "Use this to override the maximum size for fragmented messages")
set(BATCH_UNICAST_SIZE 2048 CACHE STRING "Use this to override the maximum unicast batch size")
set(BATCH_MULTICAST_SIZE 2048 CACHE STRING "Use this to override the maximum multicast batch size")
set(Z_CONFIG_SOCKET_TIMEOUT 100 CACHE STRING "Default socket timeout in milliseconds")
set(Z_TRANSPORT_LEASE 10000 CACHE STRING "Link lease duration in milliseconds to announce to other zenoh nodes")
set(Z_TRANSPORT_LEASE_EXPIRE_FACTOR 3 CACHE STRING "Default session lease expire factor.")
set(Z_RUNTIME_MAX_TASKS 64 CACHE STRING "Maximum number of tasks in zenoh-pico's runtime")
set(Z_RUNTIME_IDLE_READ_TASK_SLEEP 0 CACHE STRING "Idle read task sleep duration in milliseconds.")
set(Z_TRANSPORT_ACCEPT_TIMEOUT 1000 CACHE STRING "Link accept timeout in P2P mode in milliseconds")
set(Z_TRANSPORT_CONNECT_TIMEOUT 10000 CACHE STRING "Link connect timeout in P2P mode in milliseconds")

set(Z_FEATURE_UNSTABLE_API 0 CACHE STRING "Toggle unstable Zenoh-C API")
set(Z_FEATURE_CONNECTIVITY 0 CACHE STRING "Toggle connectivity status/events API (unstable)")
set(Z_FEATURE_PUBLICATION 1 CACHE STRING "Toggle publication feature")
set(Z_FEATURE_ADVANCED_PUBLICATION 0 CACHE STRING "Toggle advanced publication feature")
set(Z_FEATURE_SUBSCRIPTION 1 CACHE STRING "Toggle subscription feature")
set(Z_FEATURE_ADVANCED_SUBSCRIPTION 0 CACHE STRING "Toggle advanced subscription feature")
set(Z_FEATURE_QUERY 1 CACHE STRING "Toggle query feature")
set(Z_FEATURE_QUERYABLE 1 CACHE STRING "Toggle queryable feature")
set(Z_FEATURE_LIVELINESS 1 CACHE STRING "Toggle liveliness feature")
set(Z_FEATURE_INTEREST 1 CACHE STRING "Toggle interests")
set(Z_FEATURE_FRAGMENTATION 1 CACHE STRING "Toggle fragmentation")
set(Z_FEATURE_ENCODING_VALUES 1 CACHE STRING "Toggle encoding values")
set(Z_FEATURE_MULTI_THREAD 1 CACHE STRING "Toggle multithread")

set(Z_FEATURE_LINK_TCP 1 CACHE STRING "Toggle TCP links")
set(Z_FEATURE_LINK_BLUETOOTH 0 CACHE STRING "Toggle Bluetooth links")
set(Z_FEATURE_LINK_WS 0 CACHE STRING "Toggle WebSocket links")
set(Z_FEATURE_LINK_SERIAL 0 CACHE STRING "Toggle Serial links")
set(Z_FEATURE_LINK_SERIAL_USB 0 CACHE STRING "Toggle Serial USB links")
set(Z_FEATURE_LINK_TLS 0 CACHE STRING "Toggle TLS links")
set(Z_FEATURE_SCOUTING 1 CACHE STRING "Toggle UDP scouting")
set(Z_FEATURE_LINK_UDP_MULTICAST 1 CACHE STRING "Toggle UDP multicast links")
set(Z_FEATURE_LINK_UDP_UNICAST 1 CACHE STRING "Toggle UDP unicast links")
set(Z_FEATURE_MULTICAST_TRANSPORT 1 CACHE STRING "Toggle multicast transport")
set(Z_FEATURE_UNICAST_TRANSPORT 1 CACHE STRING "Toggle unicast transport")
set(Z_FEATURE_RAWETH_TRANSPORT 0 CACHE STRING "Toggle raw ethernet transport")
set(Z_FEATURE_TCP_NODELAY 1 CACHE STRING "Toggle TCP_NODELAY")
set(Z_FEATURE_LOCAL_SUBSCRIBER 0 CACHE STRING "Toggle local subscriptions")
set(Z_FEATURE_SESSION_CHECK 1 CACHE STRING "Toggle publisher/querier session check")
set(Z_FEATURE_BATCHING 1 CACHE STRING "Toggle batching")
set(Z_FEATURE_BATCH_TX_MUTEX 0 CACHE STRING "Toggle tx mutex lock at a batch level")
set(Z_FEATURE_BATCH_PEER_MUTEX 0 CACHE STRING "Toggle peer mutex lock at a batch level")
set(Z_FEATURE_MATCHING 1 CACHE STRING "Toggle matching feature")
set(Z_FEATURE_RX_CACHE 0 CACHE STRING "Toggle RX_CACHE")
set(Z_FEATURE_UNICAST_PEER 1 CACHE STRING "Toggle Unicast peer mode")
set(Z_FEATURE_AUTO_RECONNECT 1 CACHE STRING "Toggle automatic reconnection")
set(Z_FEATURE_MULTICAST_DECLARATIONS 0 CACHE STRING "Toggle multicast resource declarations")
set(Z_FEATURE_LOCAL_QUERYABLE 0 CACHE STRING "Toggle local queriables")
set(Z_FEATURE_ADMIN_SPACE 0 CACHE STRING "Toggle admin space support")

# Add a warning message if someone tries to enable Z_FEATURE_LINK_SERIAL_USB directly
if(Z_FEATURE_LINK_SERIAL_USB AND NOT Z_FEATURE_UNSTABLE_API)
  message(WARNING "Z_FEATURE_LINK_SERIAL_USB can only be enabled when Z_FEATURE_UNSTABLE_API is also enabled. Disabling Z_FEATURE_LINK_SERIAL_USB.")
  set(Z_FEATURE_LINK_SERIAL_USB 0 CACHE STRING "Toggle Serial USB links" FORCE)
endif()

if(Z_FEATURE_LINK_WS AND NOT ZP_SYSTEM_LAYER STREQUAL "emscripten")
  message(FATAL_ERROR "Z_FEATURE_LINK_WS is currently only supported on the emscripten platform.")
endif()

if(Z_FEATURE_CONNECTIVITY AND NOT Z_FEATURE_UNSTABLE_API)
  message(WARNING "Z_FEATURE_CONNECTIVITY can only be enabled when Z_FEATURE_UNSTABLE_API is also enabled. Disabling Z_FEATURE_CONNECTIVITY.")
  set(Z_FEATURE_CONNECTIVITY 0 CACHE STRING "Toggle connectivity status/events API (unstable)" FORCE)
endif()

if(Z_FEATURE_MATCHING AND NOT Z_FEATURE_INTEREST)
  message(STATUS "Z_FEATURE_MATCHING can only be enabled when Z_FEATURE_INTEREST is also enabled. Disabling Z_FEATURE_MATCHING.")
  set(Z_FEATURE_MATCHING 0 CACHE STRING "Toggle matching feature" FORCE)
endif()

if(Z_FEATURE_SCOUTING AND NOT Z_FEATURE_LINK_UDP_UNICAST)
  message(STATUS "Z_FEATURE_SCOUTING disabled because Z_FEATURE_LINK_UDP_UNICAST disabled")
  set(Z_FEATURE_SCOUTING 0 CACHE STRING "Toggle scouting feature" FORCE)
endif()

if(ZP_SYSTEM_LAYER STREQUAL "freertos_plus_tcp" AND Z_FEATURE_LINK_UDP_MULTICAST)
  message(STATUS "Z_FEATURE_LINK_UDP_MULTICAST disabled for FreeRTOS-Plus-TCP system layer")
  set(Z_FEATURE_LINK_UDP_MULTICAST 0 CACHE STRING "Toggle UDP multicast links" FORCE)
endif()

if(Z_FEATURE_ADVANCED_PUBLICATION AND (NOT Z_FEATURE_UNSTABLE_API OR NOT Z_FEATURE_PUBLICATION OR NOT Z_FEATURE_LIVELINESS))
  message(WARNING "Z_FEATURE_ADVANCED_PUBLICATION can only be enabled when Z_FEATURE_UNSTABLE_API, Z_FEATURE_PUBLICATION and Z_FEATURE_LIVELINESS is also enabled. Disabling Z_FEATURE_ADVANCED_PUBLICATION.")
  set(Z_FEATURE_ADVANCED_PUBLICATION 0 CACHE STRING "Toggle advanced publication feature" FORCE)
endif()


if(Z_FEATURE_ADMIN_SPACE AND NOT Z_FEATURE_UNSTABLE_API)
  message(WARNING "Z_FEATURE_ADMIN_SPACE can only be enabled when Z_FEATURE_UNSTABLE_API is enabled. Disabling Z_FEATURE_ADMIN_SPACE.")
  set(Z_FEATURE_ADMIN_SPACE 0 CACHE STRING "Toggle admin space support" FORCE)
endif()

if(Z_FEATURE_ADMIN_SPACE AND NOT Z_FEATURE_QUERYABLE)
  message(WARNING "Z_FEATURE_ADMIN_SPACE can only be enabled when Z_FEATURE_QUERYABLE is enabled. Disabling Z_FEATURE_ADMIN_SPACE.")
  set(Z_FEATURE_ADMIN_SPACE 0 CACHE STRING "Toggle admin space support" FORCE)
endif()


configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/zenoh-pico/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/zenoh-pico/config.h
  @ONLY
)
