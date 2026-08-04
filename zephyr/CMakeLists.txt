if(NOT CONFIG_ZENOH_PICO)
	return()
endif()

zephyr_include_directories(../include)
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
zephyr_library()

zephyr_compile_definitions(ZENOH_ZEPHYR)

# Zenoh pico feature configuration options
function(assign_kconfig_bool feature text)
  string(REPLACE Z_FEATURE CONFIG_ZENOH_PICO kconfig ${feature})
  if(${kconfig})
      set(${feature} 1 CACHE STRING ${text} FORCE)
  else()
      set(${feature} 0 CACHE STRING ${text} FORCE)
  endif()
endfunction()

assign_kconfig_bool(Z_FEATURE_ADMIN_SPACE "Toggle admin space support")
assign_kconfig_bool(Z_FEATURE_ADVANCED_PUBLICATION "Toggle advanced publication feature")
assign_kconfig_bool(Z_FEATURE_ADVANCED_SUBSCRIPTION "Toggle advanced subscription feature")
assign_kconfig_bool(Z_FEATURE_AUTO_RECONNECT "Toggle automatic reconnection")
assign_kconfig_bool(Z_FEATURE_BATCHING "Toggle batching")
assign_kconfig_bool(Z_FEATURE_BATCH_PEER_MUTEX "Toggle peer mutex lock at a batch level")
assign_kconfig_bool(Z_FEATURE_BATCH_TX_MUTEX "Toggle tx mutex lock at a batch level")
assign_kconfig_bool(Z_FEATURE_CONNECTIVITY "Toggle connectivity status/events API (unstable)")
assign_kconfig_bool(Z_FEATURE_ENCODING_VALUES "Toggle encoding values")
assign_kconfig_bool(Z_FEATURE_FRAGMENTATION "Toggle fragmentation")
assign_kconfig_bool(Z_FEATURE_INTEREST "Toggle interests")
assign_kconfig_bool(Z_FEATURE_LINK_BLUETOOTH "Toggle Bluetooth links")
assign_kconfig_bool(Z_FEATURE_LINK_SERIAL "Toggle Serial links")
assign_kconfig_bool(Z_FEATURE_LINK_SERIAL_USB "Toggle Serial USB links")
assign_kconfig_bool(Z_FEATURE_LINK_TCP "Toggle TCP links")
assign_kconfig_bool(Z_FEATURE_LINK_TLS "Toggle TLS links")
assign_kconfig_bool(Z_FEATURE_LINK_UDP_MULTICAST "Toggle UDP multicast links")
assign_kconfig_bool(Z_FEATURE_LINK_UDP_UNICAST "Toggle UDP unicast links")
assign_kconfig_bool(Z_FEATURE_LINK_WS "Toggle WebSocket links")
assign_kconfig_bool(Z_FEATURE_LIVELINESS "Toggle liveliness feature")
assign_kconfig_bool(Z_FEATURE_LOCAL_QUERYABLE "Toggle local queriables")
assign_kconfig_bool(Z_FEATURE_LOCAL_SUBSCRIBER "Toggle local subscriptions")
assign_kconfig_bool(Z_FEATURE_MATCHING "Toggle matching feature")
assign_kconfig_bool(Z_FEATURE_MULTICAST_DECLARATIONS "Toggle multicast resource declarations")
assign_kconfig_bool(Z_FEATURE_MULTICAST_TRANSPORT "Toggle multicast transport")
assign_kconfig_bool(Z_FEATURE_MULTI_THREAD "Toggle multithread")
assign_kconfig_bool(Z_FEATURE_PUBLICATION "Toggle publication feature")
assign_kconfig_bool(Z_FEATURE_QUERY "Toggle query feature")
assign_kconfig_bool(Z_FEATURE_QUERYABLE "Toggle queryable feature")
assign_kconfig_bool(Z_FEATURE_RAWETH_TRANSPORT "Toggle raw ethernet transport")
assign_kconfig_bool(Z_FEATURE_RX_CACHE "Toggle RX_CACHE")
assign_kconfig_bool(Z_FEATURE_SCOUTING "Toggle UDP scouting")
assign_kconfig_bool(Z_FEATURE_SESSION_CHECK "Toggle publisher/querier session check")
assign_kconfig_bool(Z_FEATURE_SUBSCRIPTION "Toggle subscription feature")
assign_kconfig_bool(Z_FEATURE_TCP_NODELAY "Toggle TCP_NODELAY")
assign_kconfig_bool(Z_FEATURE_UNICAST_PEER "Toggle Unicast peer mode")
assign_kconfig_bool(Z_FEATURE_UNICAST_TRANSPORT "Toggle unicast transport")
assign_kconfig_bool(Z_FEATURE_UNSTABLE_API "Toggle unstable Zenoh-C API")

function(assign_kconfig_symbol setting text)
  string(REPLACE "Z_" "" kconfig ${setting})
  set(${setting} CONFIG_ZENOH_PICO_${kconfig} CACHE STRING ${text} FORCE)
endfunction()

assign_kconfig_symbol(FRAG_MAX_SIZE "Use this to override the maximum size for fragmented messages")
assign_kconfig_symbol(BATCH_UNICAST_SIZE "Use this to override the maximum unicast batch size")
assign_kconfig_symbol(BATCH_MULTICAST_SIZE "Use this to override the maximum multicast batch size")
assign_kconfig_symbol(Z_CONFIG_SOCKET_TIMEOUT "Default socket timeout in milliseconds")
assign_kconfig_symbol(Z_TRANSPORT_LEASE "Link lease duration in milliseconds to announce to other zenoh nodes")
assign_kconfig_symbol(Z_TRANSPORT_LEASE_EXPIRE_FACTOR "Default session lease expire factor.")
assign_kconfig_symbol(Z_RUNTIME_MAX_TASKS "Maximum number of tasks in zenoh-pico's runtime")
assign_kconfig_symbol(Z_RUNTIME_IDLE_READ_TASK_SLEEP "Idle read task sleep duration in milliseconds.")
assign_kconfig_symbol(Z_TRANSPORT_ACCEPT_TIMEOUT "Link accept timeout in P2P mode in milliseconds")
assign_kconfig_symbol(Z_TRANSPORT_CONNECT_TIMEOUT "Link connect timeout in P2P mode in milliseconds")
assign_kconfig_symbol(Z_MAX_KEYEXPR_LENGTH "Maximum key expression length accepted by zenoh-pico.")

set(ZENOH_PICO_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/feature_config.cmake)

file(GLOB_RECURSE Sources
  "../src/api/*.c"
  "../src/collections/*.c"
  "../src/link/*.c"
  "../src/net/*.c"
  "../src/protocol/*.c"
  "../src/runtime/*.c"
  "../src/session/*.c"
  "../src/transport/*.c"
  "../src/utils/*.c"
  "../src/system/common/*.c"
)

file (GLOB Sources_Zephyr "../src/system/zephyr/*.c")
list(APPEND Sources ${Sources_Zephyr})
zephyr_library_sources(${Sources})
