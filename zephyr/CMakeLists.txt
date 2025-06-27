if(NOT CONFIG_ZENOH_PICO)
	return()
endif()

zephyr_include_directories(../include)
zephyr_library()

zephyr_compile_definitions(ZENOH_ZEPHYR)
zephyr_compile_definitions(ZENOH_DEBUG=0)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../version.txt version)
set(ZENOH_PICO ${PROJECT_VERSION})
set(ZENOH_PICO_MAJOR ${PROJECT_VERSION_MAJOR})
set(ZENOH_PICO_MINOR ${PROJECT_VERSION_MINOR})
set(ZENOH_PICO_PATCH ${PROJECT_VERSION_PATCH})
set(ZENOH_PICO_TWEAK ${PROJECT_VERSION_TWEAK})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/zenoh-pico.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/zenoh-pico.h
  @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/../library.json.in
  ${CMAKE_CURRENT_SOURCE_DIR}/../library.json
  @ONLY
)

# Zenoh pico feature configuration options
set(FRAG_MAX_SIZE 4096 CACHE STRING "Use this to override the maximum size for fragmented messages")
set(BATCH_UNICAST_SIZE 2048 CACHE STRING "Use this to override the maximum unicast batch size")
set(BATCH_MULTICAST_SIZE 2048 CACHE STRING "Use this to override the maximum multicast batch size")
set(Z_CONFIG_SOCKET_TIMEOUT 100 CACHE STRING "Default socket timeout in milliseconds")

set(Z_FEATURE_UNSTABLE_API 0 CACHE STRING "Toggle unstable Zenoh-C API")
set(Z_FEATURE_PUBLICATION CONFIG_ZENOH_PICO_PUBLICATION CACHE STRING "Toggle publication feature")
set(Z_FEATURE_SUBSCRIPTION CONFIG_ZENOH_PICO_SUBSCRIPTION CACHE STRING "Toggle subscription feature")
set(Z_FEATURE_QUERY CONFIG_ZENOH_PICO_QUERY CACHE STRING "Toggle query feature")
set(Z_FEATURE_QUERYABLE CONFIG_ZENOH_PICO_QUERYABLE CACHE STRING "Toggle queryable feature")
set(Z_FEATURE_LIVELINESS CONFIG_ZENOH_PICO_LIVELINESS CACHE STRING "Toggle liveliness feature")
set(Z_FEATURE_INTEREST CONFIG_ZENOH_PICO_INTEREST CACHE STRING "Toggle interests")
set(Z_FEATURE_FRAGMENTATION CONFIG_ZENOH_PICO_FRAGMENTATION CACHE STRING "Toggle fragmentation")
set(Z_FEATURE_ENCODING_VALUES CONFIG_ZENOH_PICO_ENCODING_VALUES CACHE STRING "Toggle encoding values")
set(Z_FEATURE_MULTI_THREAD 1 CACHE STRING "Toggle multithread")

set(Z_FEATURE_LINK_TCP CONFG_ZENOH_PICO_LINK_TCP CACHE STRING "Toggle TCP links")
set(Z_FEATURE_LINK_BLUETOOTH 0 CACHE STRING "Toggle Bluetooth links")
set(Z_FEATURE_LINK_WS 0 CACHE STRING "Toggle WebSocket links")
set(Z_FEATURE_LINK_SERIAL CONFIG_ZENOH_PICO_LINK_SERIAL CACHE STRING "Toggle Serial links")
set(Z_FEATURE_LINK_SERIAL_USB 0 CACHE STRING "Toggle Serial USB links")
set(Z_FEATURE_SCOUTING_UDP CONFIG_ZENOH_PICO_SCOUTING_UDP CACHE STRING "Toggle UDP scouting")
set(Z_FEATURE_LINK_UDP_MULTICAST CONFIG_ZENOH_PICO_LINK_UDP_MULTICAST CACHE STRING "Toggle UDP multicast links")
set(Z_FEATURE_LINK_UDP_UNICAST CONFIG_ZENOH_PICO_LINK_UDP_UNICAST CACHE STRING "Toggle UDP unicast links")
set(Z_FEATURE_MULTICAST_TRANSPORT CONFIG_ZENOH_PICO_MULTICAST_TRANSPORT CACHE STRING "Toggle multicast transport")
set(Z_FEATURE_UNICAST_TRANSPORT CONFIG_ZENOH_PICO_UNICAST_TRANSPORT CACHE STRING "Toggle unicast transport")
set(Z_FEATURE_RAWETH_TRANSPORT 0 CACHE STRING "Toggle raw ethernet transport")
set(Z_FEATURE_TCP_NODELAY 1 CACHE STRING "Toggle TCP_NODELAY")
set(Z_FEATURE_LOCAL_SUBSCRIBER 0 CACHE STRING "Toggle local subscriptions")
set(Z_FEATURE_SESSION_CHECK 1 CACHE STRING "Toggle publisher/querier session check")
set(Z_FEATURE_BATCHING 1 CACHE STRING "Toggle batching")
set(Z_FEATURE_MATCHING 1 CACHE STRING "Toggle matching feature")
set(Z_FEATURE_RX_CACHE 0 CACHE STRING "Toggle RX_CACHE")
set(Z_FEATURE_UNICAST_PEER CONFIG_ZENOH_PICO_UNICAST_PEER_MODE CACHE STRING "Toggle Unicast peer mode")
set(Z_FEATURE_AUTO_RECONNECT CONFIG_ZENOH_PICO_AUTO_RECONNECT CACHE STRING "Toggle automatic reconnection")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/zenoh-pico/config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/../include/zenoh-pico/config.h
  @ONLY
)

file(GLOB_RECURSE Sources
  "../src/api/*.c"
  "../src/collections/*.c"
  "../src/link/*.c"
  "../src/net/*.c"
  "../src/protocol/*.c"
  "../src/session/*.c"
  "../src/transport/*.c"
  "../src/utils/*.c"
  "../src/system/common/*.c"
)

file (GLOB Sources_Zephyr "../src/system/zephyr/*.c")
list(APPEND Sources ${Sources_Zephyr})

zephyr_library_sources(${Sources})
