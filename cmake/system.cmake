function(zp_load_system_layer_descriptor name out_var)
  set(_zp_descriptor_file "")
  if(NOT name STREQUAL "")
    foreach(_zp_system_layer_dir IN LISTS ZP_SYSTEM_LAYER_DIRS)
      if(EXISTS "${_zp_system_layer_dir}/${name}.cmake")
        set(_zp_descriptor_file "${_zp_system_layer_dir}/${name}.cmake")
        break()
      endif()
    endforeach()
  endif()
  set(${out_var} "${_zp_descriptor_file}" PARENT_SCOPE)
endfunction()

function(zp_system_layer_descriptor_has_builtin_payload out_var)
  set(_zp_has_builtin_payload OFF)
  foreach(_zp_payload_var IN ITEMS
          ZP_SYSTEM_LAYER_SOURCE_FILES
          ZP_SYSTEM_LAYER_INCLUDE_DIRS
          ZP_SYSTEM_LAYER_COMPILE_OPTIONS
          ZP_SYSTEM_LAYER_LINK_LIBRARIES)
    if(NOT "${${_zp_payload_var}}" STREQUAL "")
      set(_zp_has_builtin_payload ON)
      break()
    endif()
  endforeach()
  set(${out_var} "${_zp_has_builtin_payload}" PARENT_SCOPE)
endfunction()

function(zp_resolve_system_layer name)
  set(_zp_system_imported_target "")
  set(_zp_system_source_files "")
  set(_zp_system_include_dirs "")
  set(_zp_system_compile_definitions "")
  set(_zp_system_platform_header "")
  set(_zp_system_compile_options "")
  set(_zp_system_link_libraries "")

  if(NOT name STREQUAL "")
    set(ZP_SYSTEM_LAYER_SOURCE_FILES "")
    set(ZP_SYSTEM_LAYER_INCLUDE_DIRS "")
    set(ZP_SYSTEM_LAYER_COMPILE_DEFINITIONS "")
    set(ZP_SYSTEM_LAYER_PLATFORM_HEADER "")
    set(ZP_SYSTEM_LAYER_COMPILE_OPTIONS "")
    set(ZP_SYSTEM_LAYER_LINK_LIBRARIES "")
    set(ZP_SYSTEM_LAYER_IMPORTED_TARGET "")

    zp_load_system_layer_descriptor("${name}" _zp_system_descriptor_file)
    if("${_zp_system_descriptor_file}" STREQUAL "")
      string(REPLACE ";" ", " _zp_system_layer_dirs_text "${ZP_SYSTEM_LAYER_DIRS}")
      message(FATAL_ERROR
              "Unknown system layer component: ${name}. Searched in: ${_zp_system_layer_dirs_text}")
    endif()

    include("${_zp_system_descriptor_file}")
    zp_system_layer_descriptor_has_builtin_payload(_zp_system_has_builtin_payload)
    if(NOT "${ZP_SYSTEM_LAYER_IMPORTED_TARGET}" STREQUAL "" AND _zp_system_has_builtin_payload)
      message(FATAL_ERROR
              "System layer descriptor ${_zp_system_descriptor_file} may not define both "
              "ZP_SYSTEM_LAYER_IMPORTED_TARGET and built-in source/include/option/link payload")
    endif()

    set(_zp_system_compile_definitions "${ZP_SYSTEM_LAYER_COMPILE_DEFINITIONS}")
    set(_zp_system_platform_header "${ZP_SYSTEM_LAYER_PLATFORM_HEADER}")

    if(NOT "${ZP_SYSTEM_LAYER_IMPORTED_TARGET}" STREQUAL "")
      if(NOT TARGET "${ZP_SYSTEM_LAYER_IMPORTED_TARGET}")
        message(FATAL_ERROR
                "System layer descriptor ${_zp_system_descriptor_file} references missing target "
                "${ZP_SYSTEM_LAYER_IMPORTED_TARGET}")
      endif()
      get_property(_zp_is_imported TARGET "${ZP_SYSTEM_LAYER_IMPORTED_TARGET}" PROPERTY IMPORTED)
      if(NOT _zp_is_imported)
        message(FATAL_ERROR
                "System layer descriptor ${_zp_system_descriptor_file} requires "
                "ZP_SYSTEM_LAYER_IMPORTED_TARGET to name an imported/exported package target. "
                "Local targets created directly inside package config files are not supported "
                "because they break static install/export paths.")
      endif()
      set(_zp_system_imported_target "${ZP_SYSTEM_LAYER_IMPORTED_TARGET}")
    else()
      set(_zp_system_source_files "${ZP_SYSTEM_LAYER_SOURCE_FILES}")
      set(_zp_system_include_dirs "${ZP_SYSTEM_LAYER_INCLUDE_DIRS}")
      set(_zp_system_compile_options "${ZP_SYSTEM_LAYER_COMPILE_OPTIONS}")
      set(_zp_system_link_libraries "${ZP_SYSTEM_LAYER_LINK_LIBRARIES}")
    endif()
  endif()

  set(ZP_SYSTEM_IMPORTED_TARGET "${_zp_system_imported_target}" PARENT_SCOPE)
  set(ZP_SYSTEM_SOURCE_FILES "${_zp_system_source_files}" PARENT_SCOPE)
  set(ZP_SYSTEM_INCLUDE_DIRS "${_zp_system_include_dirs}" PARENT_SCOPE)
  set(ZP_SYSTEM_COMPILE_DEFINITIONS "${_zp_system_compile_definitions}" PARENT_SCOPE)
  set(ZP_SYSTEM_PLATFORM_HEADER "${_zp_system_platform_header}" PARENT_SCOPE)
  set(ZP_SYSTEM_COMPILE_OPTIONS "${_zp_system_compile_options}" PARENT_SCOPE)
  set(ZP_SYSTEM_LINK_LIBRARIES "${_zp_system_link_libraries}" PARENT_SCOPE)
endfunction()

set(ZP_SYSTEM_LAYER_DIRS "")
