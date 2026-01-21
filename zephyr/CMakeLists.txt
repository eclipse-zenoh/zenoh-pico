if(NOT CONFIG_ZENOH_PICO)
	return()
endif()

zephyr_include_directories(../include)
zephyr_library()

zephyr_compile_definitions(ZENOH_ZEPHYR)

# Read version file and split it on '.' (dot) to extract components
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../version.txt version)
string(STRIP "${version}" version)
string(REPLACE "." ";" version_list "${version}")

# Populate the zenoh-pico version variables
set(ZENOH_PICO ${version})
list(POP_FRONT version_list ZENOH_PICO_MAJOR ZENOH_PICO_MINOR ZENOH_PICO_PATCH ZENOH_PICO_TWEAK)

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

function(assign_kconfig feature text)
  string(REPLACE Z_FEATURE CONFIG_ZENOH_PICO kconfig ${feature})
  set(${feature} ${kconfig} CACHE STRING ${text})
endfunction()

# Zenoh pico feature configuration options
assign_kconfig(Z_FEATURE_LINK_SERIAL "Toggle Serial links")
assign_kconfig(Z_FEATURE_MULTI_THREAD "Toggle multithread")
assign_kconfig(Z_FEATURE_PUBLICATION "Toggle publication feature")
assign_kconfig(Z_FEATURE_SUBSCRIPTION "Toggle subscription feature")
assign_kconfig(Z_FEATURE_QUERY "Toggle query feature")
assign_kconfig(Z_FEATURE_QUERYABLE "Toggle queryable feature")
assign_kconfig(Z_FEATURE_RAWETH_TRANSPORT C"Toggle raw ethernet transport")
assign_kconfig(Z_FEATURE_LINK_TCP "Toggle TCP links")
assign_kconfig(Z_FEATURE_LINK_UDP_UNICAST "Toggle UDP unicast links")
assign_kconfig(Z_FEATURE_LINK_UDP_MULTICAST "Toggle UDP multicast links")
assign_kconfig(Z_FEATURE_SCOUTING "Toggle UDP scouting")
assign_kconfig(Z_FEATURE_LINK_WS "Toggle WebSocket links")
assign_kconfig(Z_FEATURE_MULTICAST_TRANSPORT "Toggle multicast transport")
assign_kconfig(Z_FEATURE_UNICAST_TRANSPORT "Toggle unicast transport")
assign_kconfig(Z_FEATURE_UNICAST_PEER "Toggle Unicast peer mode")
assign_kconfig(Z_FEATURE_AUTO_RECONNECT "Toggle automatic reconnection")
assign_kconfig(Z_FEATURE_INTEREST "Toggle interests protocol")

set(ZENOH_PICO_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/feature_config.cmake)

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
