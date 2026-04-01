cmake_minimum_required(VERSION 3.14)
project(zenohpico_mylinux LANGUAGES C)

include(GNUInstallDirs)

set(ZENOH_PICO_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../../.."
    CACHE PATH
    "Path to the zenoh-pico source tree")
get_filename_component(ZENOH_PICO_SOURCE_DIR "${ZENOH_PICO_SOURCE_DIR}" ABSOLUTE)

if(NOT EXISTS "${ZENOH_PICO_SOURCE_DIR}/include/zenoh-pico.h")
  message(FATAL_ERROR
          "ZENOH_PICO_SOURCE_DIR must point to a zenoh-pico source tree; "
          "missing include/zenoh-pico.h in ${ZENOH_PICO_SOURCE_DIR}")
endif()

add_library(mylinux_system STATIC "${ZENOH_PICO_SOURCE_DIR}/src/system/unix/system.c")
set_target_properties(mylinux_system PROPERTIES
                      POSITION_INDEPENDENT_CODE ON
                      EXPORT_NAME system)
target_include_directories(mylinux_system PRIVATE "${ZENOH_PICO_SOURCE_DIR}/include")
target_compile_definitions(mylinux_system PRIVATE ZENOH_LINUX)

add_library(mylinux_serial_uart STATIC "${ZENOH_PICO_SOURCE_DIR}/src/link/backend/serial/tty_posix.c")
set_target_properties(mylinux_serial_uart PROPERTIES
                      POSITION_INDEPENDENT_CODE ON
                      EXPORT_NAME serial_uart)
target_include_directories(mylinux_serial_uart PRIVATE "${ZENOH_PICO_SOURCE_DIR}/include")
target_compile_definitions(mylinux_serial_uart PRIVATE ZENOH_LINUX)

install(TARGETS mylinux_system mylinux_serial_uart
        EXPORT zenohpicoMylinuxTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(EXPORT zenohpicoMylinuxTargets
        NAMESPACE mylinux::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zenohpico-mylinux")
install(FILES "${CMAKE_CURRENT_LIST_DIR}/cmake/zenohpico-mylinuxConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zenohpico-mylinux")
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/cmake/platforms"
                  "${CMAKE_CURRENT_LIST_DIR}/cmake/system"
                  "${CMAKE_CURRENT_LIST_DIR}/cmake/backends"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zenohpico-mylinux")
