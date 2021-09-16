cmake_minimum_required(VERSION 3.13.1)

add_definitions(-DSHIELD=link_board_eth)
#set(DTC_OVERLAY_FILE "/Users/user/.platformio/packages/framework-zephyr/boards/shields/link_board_eth/link_board_eth.overlay")
set(DTC_OVERLAY_FILE $ENV{ZEPHYR_BASE}/boards/shields/link_board_eth/link_board_eth.overlay)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(zenoh-pico-zephyr)

FILE(GLOB app_sources ../src/*.c*)
target_sources(app PRIVATE ${app_sources})
