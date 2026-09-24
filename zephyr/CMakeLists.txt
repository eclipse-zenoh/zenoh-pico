if(CONFIG_ZENOH_PICO)

zephyr_compile_definitions(ZENOH_ZEPHYR)
zephyr_include_directories(../include)
zephyr_library()

# Before Zephyr 4.2 the POSIX headers are only on the include path with
# CONFIG_POSIX_API, which zenoh-pico does not need.
if(NOT CONFIG_POSIX_API AND NOT CONFIG_POSIX_SYSTEM_HEADERS AND NOT CONFIG_POSIX_SYSTEM_INTERFACES)
  zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr/posix)
endif()

# Generate zenoh-pico/config.h from Kconfig. Every Z_FEATURE_<X> in the
# template maps to CONFIG_ZENOH_PICO_<X>; features without a Kconfig option
# are unsupported on Zephyr and are disabled.
set(config_template ${CMAKE_CURRENT_SOURCE_DIR}/../include/zenoh-pico/config.h.in)
file(READ ${config_template} template)
string(REGEX MATCHALL "@Z_FEATURE_[A-Z0-9_]+@" features "${template}")
list(REMOVE_DUPLICATES features)
foreach(feature ${features})
  string(REPLACE "@" "" feature ${feature})
  string(REPLACE Z_FEATURE CONFIG_ZENOH_PICO kconfig ${feature})
  if(${kconfig})
    set(${feature} 1)
  else()
    set(${feature} 0)
  endif()
endforeach()

set(Z_FEATURE_UNSTABLE_API ${CONFIG_ZENOH_PICO_UNSTABLE_API})

foreach(setting FRAG_MAX_SIZE BATCH_UNICAST_SIZE BATCH_MULTICAST_SIZE
        Z_CONFIG_SOCKET_TIMEOUT Z_TRANSPORT_LEASE Z_TRANSPORT_LEASE_EXPIRE_FACTOR
        Z_RUNTIME_MAX_TASKS Z_RUNTIME_IDLE_READ_TASK_SLEEP
        Z_TRANSPORT_ACCEPT_TIMEOUT Z_TRANSPORT_CONNECT_TIMEOUT)
  string(REGEX REPLACE "^Z_" "" kconfig ${setting})
  set(${setting} ${CONFIG_ZENOH_PICO_${kconfig}})
endforeach()

configure_file(${config_template} ${CMAKE_CURRENT_BINARY_DIR}/include/zenoh-pico/config.h @ONLY)
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)


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
endif()
