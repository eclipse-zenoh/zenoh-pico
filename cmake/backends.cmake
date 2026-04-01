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

function(zp_find_backend_descriptor_file name out_var)
  set(_zp_descriptor_file "")
  if(NOT name STREQUAL "")
    foreach(_zp_backend_dir IN ITEMS ${ARGN})
      if(EXISTS "${_zp_backend_dir}/${name}.cmake")
        set(_zp_descriptor_file "${_zp_backend_dir}/${name}.cmake")
        break()
      endif()
    endforeach()
  endif()
  set(${out_var} "${_zp_descriptor_file}" PARENT_SCOPE)
endfunction()

macro(zp_reset_backend_descriptor_vars)
  set(ZP_BACKEND_SOURCE_FILES "")
  set(ZP_BACKEND_INCLUDE_DIRS "")
  set(ZP_BACKEND_COMPILE_DEFINITIONS "")
  set(ZP_BACKEND_COMPILE_OPTIONS "")
  set(ZP_BACKEND_LINK_LIBRARIES "")
  set(ZP_BACKEND_IMPORTED_TARGET "")
  set(ZP_TCP_BACKEND_SYMBOL "")
  set(ZP_SERIAL_BACKEND_SYMBOL "")
  set(ZP_BACKEND_SOCKET_COMPONENT "")
  set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "")
  set(ZP_UDP_BACKEND_UNICAST_SYMBOL "")
  set(ZP_UDP_BACKEND_MULTICAST_SYMBOL "")
endmacro()

function(zp_backend_descriptor_has_builtin_payload out_var)
  set(_zp_has_builtin_payload OFF)
  foreach(_zp_payload_var IN ITEMS
          ZP_BACKEND_SOURCE_FILES
          ZP_BACKEND_INCLUDE_DIRS
          ZP_BACKEND_COMPILE_DEFINITIONS
          ZP_BACKEND_COMPILE_OPTIONS
          ZP_BACKEND_LINK_LIBRARIES)
    if(NOT "${${_zp_payload_var}}" STREQUAL "")
      set(_zp_has_builtin_payload ON)
      break()
    endif()
  endforeach()
  set(${out_var} "${_zp_has_builtin_payload}" PARENT_SCOPE)
endfunction()

function(zp_resolve_backend_descriptor name kind prefix symbol_var)
  set(_zp_backend_imported_target "")
  set(_zp_backend_source_files "")
  set(_zp_backend_include_dirs "")
  set(_zp_backend_compile_definitions "")
  set(_zp_backend_compile_options "")
  set(_zp_backend_link_libraries "")
  set(_zp_backend_symbol "")
  set(_zp_backend_socket_component "")
  set(_zp_backend_compatible_system_layers "")

  if(NOT name STREQUAL "")
    zp_reset_backend_descriptor_vars()
    zp_find_backend_descriptor_file("${name}" _zp_backend_descriptor_file ${ARGN})
    if("${_zp_backend_descriptor_file}" STREQUAL "")
      string(REPLACE ";" ", " _zp_backend_dirs_text "${ARGN}")
      message(FATAL_ERROR "Unknown ${kind}: ${name}. Searched in: ${_zp_backend_dirs_text}")
    endif()

    include("${_zp_backend_descriptor_file}")

    zp_backend_descriptor_has_builtin_payload(_zp_backend_has_builtin_payload)
    if(NOT "${ZP_BACKEND_IMPORTED_TARGET}" STREQUAL "" AND _zp_backend_has_builtin_payload)
      message(FATAL_ERROR
              "${kind} descriptor ${_zp_backend_descriptor_file} may not define both "
              "ZP_BACKEND_IMPORTED_TARGET and built-in payload variables")
    endif()

    if(NOT "${ZP_BACKEND_IMPORTED_TARGET}" STREQUAL "")
      if(NOT TARGET "${ZP_BACKEND_IMPORTED_TARGET}")
        message(FATAL_ERROR
                "${kind} descriptor ${_zp_backend_descriptor_file} references missing target "
                "${ZP_BACKEND_IMPORTED_TARGET}")
      endif()
      get_property(_zp_is_imported TARGET "${ZP_BACKEND_IMPORTED_TARGET}" PROPERTY IMPORTED)
      if(NOT _zp_is_imported)
        message(FATAL_ERROR
                "${kind} descriptor ${_zp_backend_descriptor_file} requires "
                "ZP_BACKEND_IMPORTED_TARGET to name an imported/exported package target. "
                "Local targets created directly inside package config files are not supported "
                "because they break static install/export paths.")
      endif()
      set(_zp_backend_imported_target "${ZP_BACKEND_IMPORTED_TARGET}")
    else()
      set(_zp_backend_source_files "${ZP_BACKEND_SOURCE_FILES}")
      set(_zp_backend_include_dirs "${ZP_BACKEND_INCLUDE_DIRS}")
      set(_zp_backend_compile_definitions "${ZP_BACKEND_COMPILE_DEFINITIONS}")
      set(_zp_backend_compile_options "${ZP_BACKEND_COMPILE_OPTIONS}")
      set(_zp_backend_link_libraries "${ZP_BACKEND_LINK_LIBRARIES}")
    endif()

    set(_zp_backend_symbol "${${symbol_var}}")
    if("${_zp_backend_symbol}" STREQUAL "")
      message(FATAL_ERROR
              "${kind} descriptor ${_zp_backend_descriptor_file} must define ${symbol_var}")
    endif()
    set(_zp_backend_socket_component "${ZP_BACKEND_SOCKET_COMPONENT}")
    set(_zp_backend_compatible_system_layers "${ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS}")
  endif()

  set("${prefix}_IMPORTED_TARGET" "${_zp_backend_imported_target}" PARENT_SCOPE)
  set("${prefix}_SOURCE_FILES" "${_zp_backend_source_files}" PARENT_SCOPE)
  set("${prefix}_INCLUDE_DIRS" "${_zp_backend_include_dirs}" PARENT_SCOPE)
  set("${prefix}_COMPILE_DEFINITIONS" "${_zp_backend_compile_definitions}" PARENT_SCOPE)
  set("${prefix}_COMPILE_OPTIONS" "${_zp_backend_compile_options}" PARENT_SCOPE)
  set("${prefix}_LINK_LIBRARIES" "${_zp_backend_link_libraries}" PARENT_SCOPE)
  set("${prefix}_SYMBOL" "${_zp_backend_symbol}" PARENT_SCOPE)
  set("${prefix}_SOCKET_COMPONENT" "${_zp_backend_socket_component}" PARENT_SCOPE)
  set("${prefix}_COMPATIBLE_SYSTEM_LAYERS" "${_zp_backend_compatible_system_layers}" PARENT_SCOPE)
endfunction()

function(zp_resolve_udp_backend name)
  set(_zp_udp_backend_imported_target "")
  set(_zp_udp_backend_source_files "")
  set(_zp_udp_backend_include_dirs "")
  set(_zp_udp_backend_compile_definitions "")
  set(_zp_udp_backend_compile_options "")
  set(_zp_udp_backend_link_libraries "")
  set(_zp_udp_backend_unicast_symbol "")
  set(_zp_udp_backend_multicast_symbol "")
  set(_zp_udp_backend_socket_component "")
  set(_zp_udp_backend_compatible_system_layers "")

  if(NOT name STREQUAL "")
    zp_reset_backend_descriptor_vars()
    zp_find_backend_descriptor_file("${name}" _zp_udp_descriptor_file ${ZP_UDP_BACKEND_DIRS})
    if("${_zp_udp_descriptor_file}" STREQUAL "")
      string(REPLACE ";" ", " _zp_udp_backend_dirs_text "${ZP_UDP_BACKEND_DIRS}")
      message(FATAL_ERROR
              "Unknown UDP transport backend: ${name}. Searched in: ${_zp_udp_backend_dirs_text}")
    endif()

    include("${_zp_udp_descriptor_file}")

    zp_backend_descriptor_has_builtin_payload(_zp_backend_has_builtin_payload)
    if(NOT "${ZP_BACKEND_IMPORTED_TARGET}" STREQUAL "" AND _zp_backend_has_builtin_payload)
      message(FATAL_ERROR
              "UDP backend descriptor ${_zp_udp_descriptor_file} may not define both "
              "ZP_BACKEND_IMPORTED_TARGET and built-in payload variables")
    endif()

    if(NOT "${ZP_BACKEND_IMPORTED_TARGET}" STREQUAL "")
      if(NOT TARGET "${ZP_BACKEND_IMPORTED_TARGET}")
        message(FATAL_ERROR
                "UDP backend descriptor ${_zp_udp_descriptor_file} references missing target "
                "${ZP_BACKEND_IMPORTED_TARGET}")
      endif()
      get_property(_zp_is_imported TARGET "${ZP_BACKEND_IMPORTED_TARGET}" PROPERTY IMPORTED)
      if(NOT _zp_is_imported)
        message(FATAL_ERROR
                "UDP backend descriptor ${_zp_udp_descriptor_file} requires "
                "ZP_BACKEND_IMPORTED_TARGET to name an imported/exported package target. "
                "Local targets created directly inside package config files are not supported "
                "because they break static install/export paths.")
      endif()
      set(_zp_udp_backend_imported_target "${ZP_BACKEND_IMPORTED_TARGET}")
    else()
      set(_zp_udp_backend_source_files "${ZP_BACKEND_SOURCE_FILES}")
      set(_zp_udp_backend_include_dirs "${ZP_BACKEND_INCLUDE_DIRS}")
      set(_zp_udp_backend_compile_definitions "${ZP_BACKEND_COMPILE_DEFINITIONS}")
      set(_zp_udp_backend_compile_options "${ZP_BACKEND_COMPILE_OPTIONS}")
      set(_zp_udp_backend_link_libraries "${ZP_BACKEND_LINK_LIBRARIES}")
    endif()

    set(_zp_udp_backend_unicast_symbol "${ZP_UDP_BACKEND_UNICAST_SYMBOL}")
    if("${_zp_udp_backend_unicast_symbol}" STREQUAL "")
      message(FATAL_ERROR
              "UDP backend descriptor ${_zp_udp_descriptor_file} must define "
              "ZP_UDP_BACKEND_UNICAST_SYMBOL")
    endif()

    zp_udp_multicast_enabled(_zp_udp_multicast_enabled)
    if(_zp_udp_multicast_enabled)
      set(_zp_udp_backend_multicast_symbol "${ZP_UDP_BACKEND_MULTICAST_SYMBOL}")
    endif()
    set(_zp_udp_backend_socket_component "${ZP_BACKEND_SOCKET_COMPONENT}")
    set(_zp_udp_backend_compatible_system_layers "${ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS}")
  endif()

  set(ZP_UDP_BACKEND_IMPORTED_TARGET "${_zp_udp_backend_imported_target}" PARENT_SCOPE)
  set(ZP_UDP_BACKEND_SOURCE_FILES "${_zp_udp_backend_source_files}" PARENT_SCOPE)
  set(ZP_UDP_BACKEND_INCLUDE_DIRS "${_zp_udp_backend_include_dirs}" PARENT_SCOPE)
  set(ZP_UDP_BACKEND_COMPILE_DEFINITIONS "${_zp_udp_backend_compile_definitions}" PARENT_SCOPE)
  set(ZP_UDP_BACKEND_COMPILE_OPTIONS "${_zp_udp_backend_compile_options}" PARENT_SCOPE)
  set(ZP_UDP_BACKEND_LINK_LIBRARIES "${_zp_udp_backend_link_libraries}" PARENT_SCOPE)
  set(ZP_UDP_BACKEND_UNICAST_SYMBOL "${_zp_udp_backend_unicast_symbol}" PARENT_SCOPE)
  set(ZP_UDP_BACKEND_MULTICAST_SYMBOL "${_zp_udp_backend_multicast_symbol}" PARENT_SCOPE)
  set(ZP_UDP_BACKEND_SOCKET_COMPONENT "${_zp_udp_backend_socket_component}" PARENT_SCOPE)
  set(ZP_UDP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "${_zp_udp_backend_compatible_system_layers}" PARENT_SCOPE)
endfunction()

macro(zp_resolve_tcp_backend name)
  zp_resolve_backend_descriptor("${name}" "TCP backend" "ZP_TCP_BACKEND"
                                "ZP_TCP_BACKEND_SYMBOL" ${ZP_TCP_BACKEND_DIRS})
endmacro()

macro(zp_resolve_serial_backend name)
  zp_resolve_backend_descriptor("${name}" "serial backend" "ZP_SERIAL_BACKEND"
                                "ZP_SERIAL_BACKEND_SYMBOL" ${ZP_SERIAL_BACKEND_DIRS})
endmacro()

function(zp_validate_backend_selection type name required_socket_component compatible_system_layers system_layer
         network socket_component)
  if("${name}" STREQUAL "")
    return()
  endif()

  if(NOT "${required_socket_component}" STREQUAL "")
    if("${socket_component}" STREQUAL "")
      message(FATAL_ERROR
              "Selected ${type} backend ${name} requires socket component "
              "${required_socket_component}, but the selected network ${network} does not provide one")
    endif()
    if(NOT "${required_socket_component}" STREQUAL "${socket_component}")
      message(FATAL_ERROR
              "Selected ${type} backend ${name} requires socket component "
              "${required_socket_component}, but selected network ${network} provides "
              "socket component ${socket_component}")
    endif()
  endif()

  if(NOT "${compatible_system_layers}" STREQUAL "")
    list(FIND compatible_system_layers "${system_layer}" _zp_system_layer_index)
    if(_zp_system_layer_index EQUAL -1)
      string(REPLACE ";" ", " _zp_system_layers_text "${compatible_system_layers}")
      message(FATAL_ERROR
              "Selected ${type} backend ${name} is not compatible with system layer "
              "${system_layer}. Compatible system layers: ${_zp_system_layers_text}")
    endif()
  endif()
endfunction()

macro(zp_platform_set_tcp_backend value)
  set(ZP_PLATFORM_DEFAULT_TCP_BACKEND "${value}")
endmacro()

macro(zp_platform_set_udp_backend value)
  set(ZP_PLATFORM_DEFAULT_UDP_BACKEND "${value}")
endmacro()

macro(zp_platform_set_serial_backend value)
  set(ZP_PLATFORM_DEFAULT_SERIAL_BACKEND "${value}")
endmacro()

set(ZP_TCP_BACKEND_DIRS "${CMAKE_CURRENT_LIST_DIR}/backends/tcp")
set(ZP_UDP_BACKEND_DIRS "${CMAKE_CURRENT_LIST_DIR}/backends/udp")
set(ZP_SERIAL_BACKEND_DIRS "${CMAKE_CURRENT_LIST_DIR}/backends/serial")
