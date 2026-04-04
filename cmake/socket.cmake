function(zp_register_socket_component)
  set(oneValueArgs NAME SOCKET_OPS_SYMBOL TARGET)
  set(multiValueArgs SOURCES INCLUDE_DIRS LINK_LIBRARIES)
  cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if("${ARG_NAME}" STREQUAL "")
    message(FATAL_ERROR "zp_register_socket_component(...) requires NAME")
  endif()

  string(MAKE_C_IDENTIFIER "${ARG_NAME}" _zp_socket_component_target_id)
  set(_zp_socket_component_backing_target "zp_socket_component_${_zp_socket_component_target_id}")
  if("${ARG_TARGET}" STREQUAL "")
    zp_make_contract_target_name("socket" "${ARG_NAME}" _zp_socket_component_target)
  else()
    set(_zp_socket_component_target "${ARG_TARGET}")
  endif()

  zp_ensure_interface_contract_target("${_zp_socket_component_target}"
                                      "${_zp_socket_component_backing_target}"
                                      _zp_socket_component_actual_target)

  if(NOT "${ARG_SOURCES}" STREQUAL "")
    target_sources("${_zp_socket_component_actual_target}" INTERFACE ${ARG_SOURCES})
  endif()
  if(NOT "${ARG_INCLUDE_DIRS}" STREQUAL "")
    target_include_directories("${_zp_socket_component_actual_target}" INTERFACE ${ARG_INCLUDE_DIRS})
  endif()
  if(NOT "${ARG_LINK_LIBRARIES}" STREQUAL "")
    target_link_libraries("${_zp_socket_component_actual_target}" INTERFACE ${ARG_LINK_LIBRARIES})
  endif()

  set_property(GLOBAL PROPERTY "ZP_SOCKET_COMPONENT_TARGET_${ARG_NAME}"
                               "${_zp_socket_component_target}")
  set_property(TARGET "${_zp_socket_component_actual_target}" PROPERTY
               ZP_SOCKET_COMPONENT_SOCKET_OPS_SYMBOL "${ARG_SOCKET_OPS_SYMBOL}")
endfunction()

function(zp_load_socket_component_descriptor name)
  if(name STREQUAL "")
    return()
  endif()

  get_property(_zp_descriptor_loaded GLOBAL PROPERTY "ZP_SOCKET_COMPONENT_DESCRIPTOR_LOADED_${name}")
  if(_zp_descriptor_loaded)
    return()
  endif()

  foreach(_zp_socket_component_dir IN LISTS ZP_SOCKET_COMPONENT_DIRS)
    if(EXISTS "${_zp_socket_component_dir}/${name}.cmake")
      set_property(GLOBAL PROPERTY "ZP_SOCKET_COMPONENT_DESCRIPTOR_LOADED_${name}" TRUE)
      include("${_zp_socket_component_dir}/${name}.cmake")
      return()
    endif()
  endforeach()
endfunction()

function(zp_resolve_socket_component name out_socket_ops_symbol out_target)
  if(name STREQUAL "")
    set(${out_socket_ops_symbol} "" PARENT_SCOPE)
    set(${out_target} "" PARENT_SCOPE)
    return()
  endif()

  zp_make_contract_target_name("socket" "${name}" _zp_socket_component_target)
  if(NOT TARGET "${_zp_socket_component_target}")
    zp_load_socket_component_descriptor("${name}")
  endif()
  if(NOT TARGET "${_zp_socket_component_target}")
    get_property(_zp_socket_component_target GLOBAL PROPERTY "ZP_SOCKET_COMPONENT_TARGET_${name}")
  endif()
  if(_zp_socket_component_target STREQUAL "" OR NOT TARGET "${_zp_socket_component_target}")
    string(REPLACE ";" ", " _zp_socket_component_dirs_text "${ZP_SOCKET_COMPONENT_DIRS}")
    message(FATAL_ERROR "Unknown socket component: ${name}. Searched in: ${_zp_socket_component_dirs_text}")
  endif()
  if(_zp_socket_component_target STREQUAL "" OR NOT TARGET "${_zp_socket_component_target}")
    message(FATAL_ERROR "Socket component ${name} did not register a valid CMake target")
  endif()

  get_target_property_if_set(_zp_socket_component_socket_ops_symbol "${_zp_socket_component_target}"
                             ZP_SOCKET_COMPONENT_SOCKET_OPS_SYMBOL)
  if(NOT DEFINED _zp_socket_component_socket_ops_symbol)
    set(_zp_socket_component_socket_ops_symbol "")
  endif()
  set(${out_socket_ops_symbol} "${_zp_socket_component_socket_ops_symbol}" PARENT_SCOPE)
  set(${out_target} "${_zp_socket_component_target}" PARENT_SCOPE)
endfunction()

set(ZP_SOCKET_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/socket")
