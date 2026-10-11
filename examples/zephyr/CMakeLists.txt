cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zenoh_pico_examples)

target_sources_ifdef(CONFIG_ZP_EXAMPLE_PUB app PRIVATE z_pub.c)
target_sources_ifdef(CONFIG_ZP_EXAMPLE_SUB app PRIVATE z_sub.c)
target_sources_ifdef(CONFIG_ZP_EXAMPLE_PULL app PRIVATE z_pull.c)
target_sources_ifdef(CONFIG_ZP_EXAMPLE_GET app PRIVATE z_get.c)
target_sources_ifdef(CONFIG_ZP_EXAMPLE_QUERYABLE app PRIVATE z_queryable.c)
target_sources_ifdef(CONFIG_ZP_EXAMPLE_SCOUT app PRIVATE z_scout.c)
