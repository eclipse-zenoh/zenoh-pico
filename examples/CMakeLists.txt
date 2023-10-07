#
# Copyright (c) 2022 ZettaScale Technology
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
# ZettaScale Zenoh Team, <zenoh@zettascale.tech>
#
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  # Settings when 'examples' is the root projet
  cmake_minimum_required(VERSION 3.20)
  project(zenohpico_examples LANGUAGES C)
  include(../cmake/helpers.cmake)
  set_default_build_type(Release)
  configure_include_project(ZENOHPICO zenohpico zenohpico ".." zenohc "https://github.com/eclipse-zenoh/zenoh-pico" "")
  add_custom_target(examples ALL)
else()
  message(STATUS "zenoh-pico examples")
  add_custom_target(examples)
endif()

function(add_example name)
  add_executable(${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY C_STANDARD 11)
  target_link_libraries(${name} zenohpico)
  add_dependencies(examples ${name})
endfunction()

if(UNIX)
  if(BUILD_WITH_CUSTOM_ALLOCATOR)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
    if(CMAKE_C_STANDARD MATCHES "99")
      add_executable(z_pub_ualloc ${PROJECT_SOURCE_DIR}/examples/unix/c99/z_pub_ualloc.c)
    else()
      add_executable(z_pub_ualloc ${PROJECT_SOURCE_DIR}/examples/unix/c11/z_pub_ualloc.c)
    endif()

    target_link_libraries(z_pub_ualloc ${Libname})
  else()
    if(CMAKE_C_STANDARD MATCHES "99")
      add_example(z_put unix/c99/z_put.c)
      add_example(z_pub unix/c99/z_pub.c)
      add_example(z_pub_st unix/c99/z_pub_st.c)
      add_example(z_sub unix/c99/z_sub.c)
      add_example(z_sub_st unix/c99/z_sub_st.c)
      add_example(z_pull unix/c99/z_pull.c)
      add_example(z_get unix/c99/z_get.c)
      add_example(z_queryable unix/c99/z_queryable.c)
      add_example(z_info unix/c99/z_info.c)
      add_example(z_scout unix/c99/z_scout.c)
      add_example(z_ping unix/c99/z_ping.c)
      add_example(z_pong unix/c99/z_pong.c)
    else()
      add_example(z_put unix/c11/z_put.c)
      add_example(z_pub unix/c11/z_pub.c)
      add_example(z_pub_st unix/c11/z_pub_st.c)
      add_example(z_sub unix/c11/z_sub.c)
      add_example(z_sub_st unix/c11/z_sub_st.c)
      add_example(z_pull unix/c11/z_pull.c)
      add_example(z_get unix/c11/z_get.c)
      add_example(z_queryable unix/c11/z_queryable.c)
      add_example(z_info unix/c11/z_info.c)
      add_example(z_scout unix/c11/z_scout.c)
      add_example(z_ping unix/c11/z_ping.c)
      add_example(z_pong unix/c11/z_pong.c)
    endif()
  endif()
elseif(MSVC)
  add_example(z_put windows/z_put.c)
  add_example(z_pub windows/z_pub.c)
  add_example(z_pub_st windows/z_pub_st.c)
  add_example(z_sub windows/z_sub.c)
  add_example(z_sub_st windows/z_sub_st.c)
  add_example(z_pull windows/z_pull.c)
  add_example(z_get windows/z_get.c)
  add_example(z_queryable windows/z_queryable.c)
  add_example(z_info windows/z_info.c)
  add_example(z_scout windows/z_scout.c)
  add_example(z_ping windows/z_ping.c)
  add_example(z_pong windows/z_pong.c)
endif()
