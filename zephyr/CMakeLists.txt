if(CONFIG_ZENOH_PICO)

zephyr_compile_definitions(ZENOH_ZEPHYR)
zephyr_include_directories(../include)
zephyr_library()

function(configure_zenoh_feature config)
  string(REPLACE CONFIG_ZENOH_PICO Z_FEATURE feature ${config})
  if(${config})
    zephyr_compile_definitions(${feature}=1)
  else()
    zephyr_compile_definitions(${feature}=0)
  endif()
endfunction()


configure_zenoh_feature(CONFIG_ZENOH_PICO_LINK_SERIAL)
configure_zenoh_feature(CONFIG_ZENOH_PICO_MULTI_THREAD)
configure_zenoh_feature(CONFIG_ZENOH_PICO_PUBLICATION)
configure_zenoh_feature(CONFIG_ZENOH_PICO_SUBSCRIPTION)
configure_zenoh_feature(CONFIG_ZENOH_PICO_QUERY)
configure_zenoh_feature(CONFIG_ZENOH_PICO_QUERYABLE)
configure_zenoh_feature(CONFIG_ZENOH_PICO_RAWETH_TRANSPORT)
configure_zenoh_feature(CONFIG_ZENOH_PICO_LINK_TCP)
configure_zenoh_feature(CONFIG_ZENOH_PICO_LINK_UDP_UNICAST)
configure_zenoh_feature(CONFIG_ZENOH_PICO_LINK_UDP_MULTICAST)
configure_zenoh_feature(CONFIG_ZENOH_PICO_SCOUTING_UDP)
configure_zenoh_feature(CONFIG_ZENOH_PICO_LINK_WS)


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
endif()
