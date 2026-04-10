function(zp_load_network_descriptor name out_var)
  set(_zp_descriptor_file "")
  if(NOT name STREQUAL "")
    foreach(_zp_network_dir IN LISTS ZP_NETWORK_DIRS)
      if(EXISTS "${_zp_network_dir}/${name}.cmake")
        set(_zp_descriptor_file "${_zp_network_dir}/${name}.cmake")
        break()
      endif()
    endforeach()
  endif()
  set(${out_var} "${_zp_descriptor_file}" PARENT_SCOPE)
endfunction()

function(zp_resolve_network name)
  set(_zp_network_source_files "")
  set(_zp_network_include_dirs "")
  set(_zp_network_compile_definitions "")
  set(_zp_network_compile_options "")
  set(_zp_network_link_libraries "")
  set(_zp_network_imported_target "")

  if(NOT name STREQUAL "")
    set(ZP_NETWORK_IMPORTED_TARGET "")
    set(ZP_NETWORK_SOURCE_FILES "")
    set(ZP_NETWORK_INCLUDE_DIRS "")
    set(ZP_NETWORK_COMPILE_DEFINITIONS "")
    set(ZP_NETWORK_COMPILE_OPTIONS "")
    set(ZP_NETWORK_LINK_LIBRARIES "")

    zp_load_network_descriptor("${name}" _zp_network_descriptor_file)
    if("${_zp_network_descriptor_file}" STREQUAL "")
      string(REPLACE ";" ", " _zp_network_dirs_text "${ZP_NETWORK_DIRS}")
      message(FATAL_ERROR "Unknown network: ${name}. Searched in: ${_zp_network_dirs_text}")
    endif()

    include("${_zp_network_descriptor_file}")

    set(_zp_network_compile_definitions "${ZP_NETWORK_COMPILE_DEFINITIONS}")
    set(_zp_network_has_builtin_component OFF)
    foreach(_zp_component_var IN ITEMS
            SOURCE_FILES
            INCLUDE_DIRS
            COMPILE_OPTIONS
            LINK_LIBRARIES)
      if(NOT "${ZP_NETWORK_${_zp_component_var}}" STREQUAL "")
        set(_zp_network_has_builtin_component ON)
        break()
      endif()
    endforeach()

    if(NOT "${ZP_NETWORK_IMPORTED_TARGET}" STREQUAL "" AND _zp_network_has_builtin_component)
      message(FATAL_ERROR
              "Network from ${_zp_network_descriptor_file} may not define both "
              "ZP_NETWORK_IMPORTED_TARGET and built-in source/include/option/link variables")
    endif()

    if(NOT "${ZP_NETWORK_IMPORTED_TARGET}" STREQUAL "")
      if(NOT TARGET "${ZP_NETWORK_IMPORTED_TARGET}")
        message(FATAL_ERROR
                "Network from ${_zp_network_descriptor_file} references missing target "
                "${ZP_NETWORK_IMPORTED_TARGET}")
      endif()
      get_property(_zp_is_imported TARGET "${ZP_NETWORK_IMPORTED_TARGET}" PROPERTY IMPORTED)
      if(NOT _zp_is_imported)
        message(FATAL_ERROR
                "Network from ${_zp_network_descriptor_file} requires "
                "ZP_NETWORK_IMPORTED_TARGET to name an imported/exported package target. "
                "Local targets created directly inside package config files are not supported "
                "because they break static install/export paths.")
      endif()
      set(_zp_network_imported_target "${ZP_NETWORK_IMPORTED_TARGET}")
    else()
      if("${ZP_NETWORK_SOURCE_FILES}" STREQUAL "")
        message(FATAL_ERROR
                "Network from ${_zp_network_descriptor_file} must define either "
                "ZP_NETWORK_IMPORTED_TARGET or ZP_NETWORK_SOURCE_FILES")
      endif()
      set(_zp_network_source_files "${ZP_NETWORK_SOURCE_FILES}")
      set(_zp_network_include_dirs "${ZP_NETWORK_INCLUDE_DIRS}")
      set(_zp_network_compile_options "${ZP_NETWORK_COMPILE_OPTIONS}")
      set(_zp_network_link_libraries "${ZP_NETWORK_LINK_LIBRARIES}")
    endif()
  endif()

  set(ZP_NETWORK_IMPORTED_TARGET "${_zp_network_imported_target}" PARENT_SCOPE)
  set(ZP_NETWORK_SOURCE_FILES "${_zp_network_source_files}" PARENT_SCOPE)
  set(ZP_NETWORK_INCLUDE_DIRS "${_zp_network_include_dirs}" PARENT_SCOPE)
  set(ZP_NETWORK_COMPILE_DEFINITIONS "${_zp_network_compile_definitions}" PARENT_SCOPE)
  set(ZP_NETWORK_COMPILE_OPTIONS "${_zp_network_compile_options}" PARENT_SCOPE)
  set(ZP_NETWORK_LINK_LIBRARIES "${_zp_network_link_libraries}" PARENT_SCOPE)
endfunction()

set(ZP_NETWORK_DIRS "")
