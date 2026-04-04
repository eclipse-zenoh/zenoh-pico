function(zp_udp_multicast_enabled out_var)
  if(DEFINED Z_FEATURE_LINK_UDP_MULTICAST)
    set(_zp_link_udp_multicast "${Z_FEATURE_LINK_UDP_MULTICAST}")
  else()
    set(_zp_link_udp_multicast "1")
  endif()
  if(DEFINED Z_FEATURE_MULTICAST_TRANSPORT)
    set(_zp_multicast_transport "${Z_FEATURE_MULTICAST_TRANSPORT}")
  else()
    set(_zp_multicast_transport "1")
  endif()

  if("${_zp_link_udp_multicast}" STREQUAL "1" AND "${_zp_multicast_transport}" STREQUAL "1")
    set(${out_var} ON PARENT_SCOPE)
  else()
    set(${out_var} OFF PARENT_SCOPE)
  endif()
endfunction()

function(zp_register_backend type)
  set(oneValueArgs NAME SYMBOL SOCKET_COMPONENT TARGET)
  set(multiValueArgs SOURCES INCLUDE_DIRS LINK_LIBRARIES COMPATIBLE_NETWORKS
                    COMPATIBLE_SOCKET_COMPONENTS
                    COMPATIBLE_SYSTEM_LAYERS)
  cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if("${ARG_NAME}" STREQUAL "")
    message(FATAL_ERROR "zp_register_backend(${type} ...) requires NAME")
  endif()
  if("${ARG_SYMBOL}" STREQUAL "")
    message(FATAL_ERROR "zp_register_backend(${type} ...) requires SYMBOL")
  endif()

  string(TOUPPER "${type}" _zp_backend_type)
  zp_backend_registry_type_to_contract_prefix("${type}" _zp_backend_contract_prefix)
  string(MAKE_C_IDENTIFIER "${type}_${ARG_NAME}" _zp_backend_target_id)
  set(_zp_backend_backing_target "zp_backend_${_zp_backend_target_id}")
  if("${ARG_TARGET}" STREQUAL "")
    zp_make_contract_target_name("${_zp_backend_contract_prefix}" "${ARG_NAME}" _zp_backend_target)
  else()
    set(_zp_backend_target "${ARG_TARGET}")
  endif()

  zp_ensure_interface_contract_target("${_zp_backend_target}" "${_zp_backend_backing_target}"
                                      _zp_backend_actual_target)

  if(NOT "${ARG_SOURCES}" STREQUAL "")
    target_sources("${_zp_backend_actual_target}" INTERFACE ${ARG_SOURCES})
  endif()
  if(NOT "${ARG_INCLUDE_DIRS}" STREQUAL "")
    target_include_directories("${_zp_backend_actual_target}" INTERFACE ${ARG_INCLUDE_DIRS})
  endif()
  if(NOT "${ARG_LINK_LIBRARIES}" STREQUAL "")
    target_link_libraries("${_zp_backend_actual_target}" INTERFACE ${ARG_LINK_LIBRARIES})
  endif()

  set_property(GLOBAL PROPERTY "ZP_${_zp_backend_type}_BACKEND_TARGET_${ARG_NAME}" "${_zp_backend_target}")
  set_property(TARGET "${_zp_backend_actual_target}" PROPERTY ZP_BACKEND_SYMBOL "${ARG_SYMBOL}")
  set_property(TARGET "${_zp_backend_actual_target}" PROPERTY ZP_BACKEND_SOCKET_COMPONENT
                                                        "${ARG_SOCKET_COMPONENT}")
  set_property(TARGET "${_zp_backend_actual_target}" PROPERTY ZP_BACKEND_COMPATIBLE_NETWORKS
                                                        "${ARG_COMPATIBLE_NETWORKS}")
  set_property(TARGET "${_zp_backend_actual_target}" PROPERTY
               ZP_BACKEND_COMPATIBLE_SOCKET_COMPONENTS "${ARG_COMPATIBLE_SOCKET_COMPONENTS}")
  set_property(TARGET "${_zp_backend_actual_target}" PROPERTY
               ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "${ARG_COMPATIBLE_SYSTEM_LAYERS}")
endfunction()

function(zp_register_udp_backend)
  set(oneValueArgs NAME UNICAST_BACKEND MULTICAST_BACKEND TARGET)
  cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

  if("${ARG_NAME}" STREQUAL "")
    message(FATAL_ERROR "zp_register_udp_backend(...) requires NAME")
  endif()
  if("${ARG_UNICAST_BACKEND}" STREQUAL "")
    message(FATAL_ERROR "zp_register_udp_backend(...) requires UNICAST_BACKEND")
  endif()

  string(MAKE_C_IDENTIFIER "udp_${ARG_NAME}" _zp_udp_backend_target_id)
  set(_zp_udp_backend_backing_target "zp_udp_backend_${_zp_udp_backend_target_id}")
  if("${ARG_TARGET}" STREQUAL "")
    zp_make_contract_target_name("udp_backend" "${ARG_NAME}" _zp_udp_backend_target)
  else()
    set(_zp_udp_backend_target "${ARG_TARGET}")
  endif()

  zp_ensure_interface_contract_target("${_zp_udp_backend_target}" "${_zp_udp_backend_backing_target}"
                                      _zp_udp_backend_actual_target)

  set_property(GLOBAL PROPERTY "ZP_UDP_BACKEND_TARGET_${ARG_NAME}" "${_zp_udp_backend_target}")
  set_property(TARGET "${_zp_udp_backend_actual_target}" PROPERTY
               ZP_UDP_BACKEND_UNICAST_BACKEND "${ARG_UNICAST_BACKEND}")
  set_property(TARGET "${_zp_udp_backend_actual_target}" PROPERTY
               ZP_UDP_BACKEND_MULTICAST_BACKEND "${ARG_MULTICAST_BACKEND}")
endfunction()

function(zp_backend_registry_type_to_contract_prefix type out_var)
  if(type STREQUAL "stream")
    set(${out_var} "tcp_backend" PARENT_SCOPE)
  elseif(type STREQUAL "datagram")
    set(${out_var} "udp_unicast_backend" PARENT_SCOPE)
  elseif(type STREQUAL "udp_multicast")
    set(${out_var} "udp_multicast_backend" PARENT_SCOPE)
  elseif(type STREQUAL "rawio")
    set(${out_var} "serial_backend" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Unknown backend type: ${type}")
  endif()
endfunction()

function(zp_get_backend_search_dirs type out_var)
  if(type STREQUAL "stream")
    set(${out_var} "${ZP_TCP_BACKEND_DIRS}" PARENT_SCOPE)
  elseif(type STREQUAL "datagram")
    set(${out_var} "${ZP_UDP_UNICAST_BACKEND_DIRS}" PARENT_SCOPE)
  elseif(type STREQUAL "udp_multicast")
    set(${out_var} "${ZP_UDP_MULTICAST_BACKEND_DIRS}" PARENT_SCOPE)
  elseif(type STREQUAL "rawio")
    set(${out_var} "${ZP_SERIAL_BACKEND_DIRS}" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Unknown backend type: ${type}")
  endif()
endfunction()

function(zp_load_udp_backend_descriptor name)
  if(name STREQUAL "")
    return()
  endif()

  get_property(_zp_descriptor_loaded GLOBAL PROPERTY "ZP_UDP_BACKEND_DESCRIPTOR_LOADED_${name}")
  if(_zp_descriptor_loaded)
    return()
  endif()

  foreach(_zp_udp_backend_dir IN LISTS ZP_UDP_BACKEND_DIRS)
    if(EXISTS "${_zp_udp_backend_dir}/${name}.cmake")
      set_property(GLOBAL PROPERTY "ZP_UDP_BACKEND_DESCRIPTOR_LOADED_${name}" TRUE)
      include("${_zp_udp_backend_dir}/${name}.cmake")
      return()
    endif()
  endforeach()
endfunction()

function(zp_resolve_udp_backend name out_unicast_backend out_multicast_backend)
  if(name STREQUAL "")
    set(${out_unicast_backend} "" PARENT_SCOPE)
    set(${out_multicast_backend} "" PARENT_SCOPE)
    return()
  endif()

  zp_make_contract_target_name("udp_backend" "${name}" _zp_udp_backend_target)
  if(NOT TARGET "${_zp_udp_backend_target}")
    zp_load_udp_backend_descriptor("${name}")
  endif()
  if(NOT TARGET "${_zp_udp_backend_target}")
    get_property(_zp_udp_backend_target GLOBAL PROPERTY "ZP_UDP_BACKEND_TARGET_${name}")
  endif()
  if("${_zp_udp_backend_target}" STREQUAL "" OR NOT TARGET "${_zp_udp_backend_target}")
    string(REPLACE ";" ", " _zp_udp_backend_dirs_text "${ZP_UDP_BACKEND_DIRS}")
    message(FATAL_ERROR
            "Unknown UDP transport backend: ${name}. Searched in: ${_zp_udp_backend_dirs_text}")
  endif()

  get_target_property_if_set(_zp_unicast_backend "${_zp_udp_backend_target}" ZP_UDP_BACKEND_UNICAST_BACKEND)
  get_target_property_if_set(_zp_multicast_backend "${_zp_udp_backend_target}" ZP_UDP_BACKEND_MULTICAST_BACKEND)
  if(NOT DEFINED _zp_unicast_backend)
    set(_zp_unicast_backend "")
  endif()
  if(NOT DEFINED _zp_multicast_backend)
    set(_zp_multicast_backend "")
  endif()
  zp_udp_multicast_enabled(_zp_udp_multicast_enabled)

  set(${out_unicast_backend} "${_zp_unicast_backend}" PARENT_SCOPE)
  if(_zp_udp_multicast_enabled)
    set(${out_multicast_backend} "${_zp_multicast_backend}" PARENT_SCOPE)
  else()
    set(${out_multicast_backend} "" PARENT_SCOPE)
  endif()
endfunction()

function(zp_backend_selection_type_to_registry_type type out_var)
  if(type STREQUAL "tcp")
    set(${out_var} "stream" PARENT_SCOPE)
  elseif(type STREQUAL "udp_unicast")
    set(${out_var} "datagram" PARENT_SCOPE)
  elseif(type STREQUAL "serial")
    set(${out_var} "rawio" PARENT_SCOPE)
  else()
    set(${out_var} "${type}" PARENT_SCOPE)
  endif()
endfunction()

function(zp_load_backend_descriptor type name)
  if(name STREQUAL "")
    return()
  endif()

  string(TOUPPER "${type}" _zp_backend_type)
  get_property(_zp_descriptor_loaded GLOBAL PROPERTY "ZP_${_zp_backend_type}_BACKEND_DESCRIPTOR_LOADED_${name}")
  if(_zp_descriptor_loaded)
    return()
  endif()

  zp_get_backend_search_dirs("${type}" _zp_backend_dirs)
  foreach(_zp_backend_dir IN LISTS _zp_backend_dirs)
    if(EXISTS "${_zp_backend_dir}/${name}.cmake")
      set_property(GLOBAL PROPERTY "ZP_${_zp_backend_type}_BACKEND_DESCRIPTOR_LOADED_${name}" TRUE)
      include("${_zp_backend_dir}/${name}.cmake")
      return()
    endif()
  endforeach()
endfunction()

function(zp_resolve_backend type name out_symbol out_target)
  if(name STREQUAL "")
    set(${out_symbol} "" PARENT_SCOPE)
    set(${out_target} "" PARENT_SCOPE)
    return()
  endif()

  string(TOUPPER "${type}" _zp_backend_type)
  zp_backend_registry_type_to_contract_prefix("${type}" _zp_backend_contract_prefix)
  zp_make_contract_target_name("${_zp_backend_contract_prefix}" "${name}" _zp_backend_target)
  if(NOT TARGET "${_zp_backend_target}")
    zp_load_backend_descriptor("${type}" "${name}")
  endif()
  if(NOT TARGET "${_zp_backend_target}")
    get_property(_zp_backend_target GLOBAL PROPERTY "ZP_${_zp_backend_type}_BACKEND_TARGET_${name}")
  endif()
  if("${_zp_backend_target}" STREQUAL "" OR NOT TARGET "${_zp_backend_target}")
    zp_get_backend_search_dirs("${type}" _zp_backend_dirs)
    string(REPLACE ";" ", " _zp_backend_dirs_text "${_zp_backend_dirs}")
    message(FATAL_ERROR "Unknown ${type} backend: ${name}. Searched in: ${_zp_backend_dirs_text}")
  endif()

  get_target_property_if_set(_zp_backend_symbol "${_zp_backend_target}" ZP_BACKEND_SYMBOL)
  if(NOT DEFINED _zp_backend_symbol)
    set(_zp_backend_symbol "")
  endif()
  set(${out_symbol} "${_zp_backend_symbol}" PARENT_SCOPE)
  set(${out_target} "${_zp_backend_target}" PARENT_SCOPE)
endfunction()

function(zp_validate_backend_selection type name backend_target system_layer network socket_component)
  if("${name}" STREQUAL "" OR "${backend_target}" STREQUAL "" OR NOT TARGET "${backend_target}")
    return()
  endif()

  zp_backend_selection_type_to_registry_type("${type}" _zp_registry_type)
  get_target_property_if_set(_zp_backend_socket_component "${backend_target}" ZP_BACKEND_SOCKET_COMPONENT)
  if(NOT DEFINED _zp_backend_socket_component)
    set(_zp_backend_socket_component "")
  endif()
  if(NOT "${_zp_backend_socket_component}" STREQUAL "")
    if("${socket_component}" STREQUAL "")
      message(FATAL_ERROR
              "Selected ${type} backend ${name} requires socket component "
              "${_zp_backend_socket_component}, but the selected network ${network} "
              "does not provide one")
    endif()
    if(NOT "${_zp_backend_socket_component}" STREQUAL "${socket_component}")
      message(FATAL_ERROR
              "Selected ${type} backend ${name} requires socket component "
              "${_zp_backend_socket_component}, but selected network ${network} provides "
              "socket component ${socket_component}")
    endif()
  endif()

  get_target_property_if_set(_zp_backend_compatible_socket_components "${backend_target}"
                             ZP_BACKEND_COMPATIBLE_SOCKET_COMPONENTS)
  if(NOT DEFINED _zp_backend_compatible_socket_components)
    set(_zp_backend_compatible_socket_components "")
  endif()
  if(NOT "${_zp_backend_compatible_socket_components}" STREQUAL "")
    if("${socket_component}" STREQUAL "")
      string(REPLACE ";" ", " _zp_socket_components_text "${_zp_backend_compatible_socket_components}")
      message(FATAL_ERROR
              "Selected ${type} backend ${name} requires one of the socket components "
              "${_zp_socket_components_text}, but the selected network ${network} does not provide one")
    endif()

    list(FIND _zp_backend_compatible_socket_components "${socket_component}" _zp_socket_component_index)
    if(_zp_socket_component_index EQUAL -1)
      string(REPLACE ";" ", " _zp_socket_components_text "${_zp_backend_compatible_socket_components}")
      message(FATAL_ERROR
              "Selected ${type} backend ${name} is not compatible with socket component "
              "${socket_component}. Compatible socket components: ${_zp_socket_components_text}")
    endif()
  endif()

  get_target_property_if_set(_zp_backend_compatible_networks "${backend_target}"
                             ZP_BACKEND_COMPATIBLE_NETWORKS)
  if(NOT DEFINED _zp_backend_compatible_networks)
    set(_zp_backend_compatible_networks "")
  endif()
  if(NOT "${_zp_backend_compatible_networks}" STREQUAL "")
    list(FIND _zp_backend_compatible_networks "${network}" _zp_network_index)
    if(_zp_network_index EQUAL -1)
      string(REPLACE ";" ", " _zp_networks_text "${_zp_backend_compatible_networks}")
      message(FATAL_ERROR
              "Selected ${type} backend ${name} is not compatible with selected network "
              "${network}. Compatible networks: ${_zp_networks_text}")
    endif()
  endif()

  get_target_property_if_set(_zp_backend_compatible_system_layers "${backend_target}"
                             ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS)
  if(NOT DEFINED _zp_backend_compatible_system_layers)
    set(_zp_backend_compatible_system_layers "")
  endif()
  if(NOT "${_zp_backend_compatible_system_layers}" STREQUAL "")
    list(FIND _zp_backend_compatible_system_layers "${system_layer}" _zp_system_layer_index)
    if(_zp_system_layer_index EQUAL -1)
      string(REPLACE ";" ", " _zp_system_layers_text "${_zp_backend_compatible_system_layers}")
      message(FATAL_ERROR
              "Selected ${type} backend ${name} is not compatible with system layer "
              "${system_layer}. Compatible system layers: ${_zp_system_layers_text}")
    endif()
  endif()
endfunction()

macro(zp_register_tcp_backend)
  zp_register_backend(stream ${ARGV})
endmacro()

macro(zp_register_udp_unicast_backend)
  zp_register_backend(datagram ${ARGV})
endmacro()

macro(zp_register_serial_backend)
  zp_register_backend(rawio ${ARGV})
endmacro()

macro(zp_register_udp_multicast_backend)
  zp_register_backend(udp_multicast ${ARGV})
endmacro()

macro(zp_resolve_tcp_backend name out_symbol out_target)
  zp_resolve_backend(stream "${name}" ${out_symbol} ${out_target})
endmacro()

macro(zp_resolve_udp_unicast_backend name out_symbol out_target)
  zp_resolve_backend(datagram "${name}" ${out_symbol} ${out_target})
endmacro()

macro(zp_resolve_serial_backend name out_symbol out_target)
  zp_resolve_backend(rawio "${name}" ${out_symbol} ${out_target})
endmacro()

macro(zp_resolve_udp_multicast_backend name out_symbol out_target)
  zp_resolve_backend(udp_multicast "${name}" ${out_symbol} ${out_target})
endmacro()

macro(zp_platform_set_tcp_backend value)
  set(ZP_PLATFORM_DEFAULT_TCP_BACKEND "${value}")
endmacro()

macro(zp_platform_set_udp_backend value)
  set(ZP_PLATFORM_DEFAULT_UDP_BACKEND "${value}")
endmacro()

macro(zp_platform_set_serial_backend value)
  set(ZP_PLATFORM_DEFAULT_SERIAL_BACKEND "${value}")
endmacro()

set(ZP_TCP_BACKEND_DIRS "${CMAKE_CURRENT_LIST_DIR}/backends/stream")
set(ZP_UDP_BACKEND_DIRS "${CMAKE_CURRENT_LIST_DIR}/backends/udp")
set(ZP_UDP_UNICAST_BACKEND_DIRS "${CMAKE_CURRENT_LIST_DIR}/backends/datagram")
set(ZP_UDP_MULTICAST_BACKEND_DIRS "${CMAKE_CURRENT_LIST_DIR}/backends/udp_multicast")
set(ZP_SERIAL_BACKEND_DIRS "${CMAKE_CURRENT_LIST_DIR}/backends/rawio")
