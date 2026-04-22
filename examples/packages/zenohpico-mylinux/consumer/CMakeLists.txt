cmake_minimum_required(VERSION 3.14)
project(zenohpico_mylinux_consumer LANGUAGES C)

find_package(zenohpico CONFIG REQUIRED)

add_executable(consumer main.c)
target_link_libraries(consumer PRIVATE zenohpico::static)
