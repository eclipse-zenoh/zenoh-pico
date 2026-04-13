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

function(zp_resolve_inline_backend input_prefix kind descriptor_label output_prefix)
  if(NOT "${${input_prefix}_COMPATIBLE_SYSTEM_LAYERS}" STREQUAL "")
    message(FATAL_ERROR
            "${kind} from ${descriptor_label} may not define "
            "${input_prefix}_COMPATIBLE_SYSTEM_LAYERS. "
            "Inline platform transport components are already tied to a single platform. "
            "Use a named backend descriptor if you need explicit system-layer compatibility metadata.")
  endif()

  set(_zp_backend_imported_target "")
  set(_zp_backend_source_files "")
  set(_zp_backend_include_dirs "")
  set(_zp_backend_compile_definitions "")
  set(_zp_backend_has_builtin_component OFF)

  foreach(_zp_component_var IN ITEMS
          SOURCE_FILES
          INCLUDE_DIRS)
    if(NOT "${${input_prefix}_${_zp_component_var}}" STREQUAL "")
      set(_zp_backend_has_builtin_component ON)
      break()
    endif()
  endforeach()

  set(_zp_backend_compile_definitions "${${input_prefix}_COMPILE_DEFINITIONS}")

  if(NOT "${${input_prefix}_IMPORTED_TARGET}" STREQUAL "" AND _zp_backend_has_builtin_component)
    message(FATAL_ERROR
            "${kind} from ${descriptor_label} may not define both "
            "${input_prefix}_IMPORTED_TARGET and built-in source/include variables")
  endif()

  if(NOT "${${input_prefix}_IMPORTED_TARGET}" STREQUAL "")
    if(NOT TARGET "${${input_prefix}_IMPORTED_TARGET}")
      message(FATAL_ERROR
              "${kind} from ${descriptor_label} references missing target "
              "${${input_prefix}_IMPORTED_TARGET}")
    endif()
    get_property(_zp_is_imported TARGET "${${input_prefix}_IMPORTED_TARGET}" PROPERTY IMPORTED)
    if(NOT _zp_is_imported)
      message(FATAL_ERROR
              "${kind} from ${descriptor_label} requires "
              "${input_prefix}_IMPORTED_TARGET to name an imported/exported package target. "
              "Local targets created directly inside package config files are not supported "
              "because they break static install/export paths.")
    endif()
    set(_zp_backend_imported_target "${${input_prefix}_IMPORTED_TARGET}")
  else()
    if("${${input_prefix}_SOURCE_FILES}" STREQUAL "")
      message(FATAL_ERROR
              "${kind} from ${descriptor_label} must define either "
              "${input_prefix}_IMPORTED_TARGET or ${input_prefix}_SOURCE_FILES")
    endif()
    set(_zp_backend_source_files "${${input_prefix}_SOURCE_FILES}")
    set(_zp_backend_include_dirs "${${input_prefix}_INCLUDE_DIRS}")
  endif()

  set("${output_prefix}_IMPORTED_TARGET" "${_zp_backend_imported_target}" PARENT_SCOPE)
  set("${output_prefix}_SOURCE_FILES" "${_zp_backend_source_files}" PARENT_SCOPE)
  set("${output_prefix}_INCLUDE_DIRS" "${_zp_backend_include_dirs}" PARENT_SCOPE)
  set("${output_prefix}_COMPILE_DEFINITIONS" "${_zp_backend_compile_definitions}" PARENT_SCOPE)
  set("${output_prefix}_COMPILE_OPTIONS" "" PARENT_SCOPE)
  set("${output_prefix}_LINK_LIBRARIES" "" PARENT_SCOPE)
  set("${output_prefix}_COMPATIBLE_SYSTEM_LAYERS" "" PARENT_SCOPE)
endfunction()

function(zp_resolve_backend name kind prefix)
  set(_zp_backend_search_dirs ${ARGN})

  if(NOT name STREQUAL "")
    list(REMOVE_DUPLICATES _zp_backend_search_dirs)

    set(ZP_BACKEND_SOURCE_FILES "")
    set(ZP_BACKEND_INCLUDE_DIRS "")
    set(ZP_BACKEND_COMPILE_DEFINITIONS "")
    set(ZP_BACKEND_COMPILE_OPTIONS "")
    set(ZP_BACKEND_LINK_LIBRARIES "")
    set(ZP_BACKEND_IMPORTED_TARGET "")
    set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS "")

    zp_find_backend_descriptor_file("${name}" _zp_backend_descriptor_file ${_zp_backend_search_dirs})
    if("${_zp_backend_descriptor_file}" STREQUAL "")
      string(REPLACE ";" ", " _zp_backend_dirs_text "${_zp_backend_search_dirs}")
      message(FATAL_ERROR "Unknown ${kind}: ${name}. Searched in: ${_zp_backend_dirs_text}")
    endif()

    include("${_zp_backend_descriptor_file}")
    set(_zp_backend_imported_target "")
    set(_zp_backend_source_files "")
    set(_zp_backend_include_dirs "")
    set(_zp_backend_compile_definitions "${ZP_BACKEND_COMPILE_DEFINITIONS}")
    set(_zp_backend_compile_options "")
    set(_zp_backend_link_libraries "")
    set(_zp_backend_compatible_system_layers "${ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS}")
    set(_zp_backend_has_builtin_component OFF)

    foreach(_zp_component_var IN ITEMS
            SOURCE_FILES
            INCLUDE_DIRS
            COMPILE_OPTIONS
            LINK_LIBRARIES)
      if(NOT "${ZP_BACKEND_${_zp_component_var}}" STREQUAL "")
        set(_zp_backend_has_builtin_component ON)
        break()
      endif()
    endforeach()

    if(NOT "${ZP_BACKEND_IMPORTED_TARGET}" STREQUAL "" AND _zp_backend_has_builtin_component)
      message(FATAL_ERROR
              "${kind} from ${_zp_backend_descriptor_file} may not define both "
              "ZP_BACKEND_IMPORTED_TARGET and built-in source/include/option/link variables")
    endif()

    if(NOT "${ZP_BACKEND_IMPORTED_TARGET}" STREQUAL "")
      if(NOT TARGET "${ZP_BACKEND_IMPORTED_TARGET}")
        message(FATAL_ERROR
                "${kind} from ${_zp_backend_descriptor_file} references missing target "
                "${ZP_BACKEND_IMPORTED_TARGET}")
      endif()
      get_property(_zp_is_imported TARGET "${ZP_BACKEND_IMPORTED_TARGET}" PROPERTY IMPORTED)
      if(NOT _zp_is_imported)
        message(FATAL_ERROR
                "${kind} from ${_zp_backend_descriptor_file} requires "
                "ZP_BACKEND_IMPORTED_TARGET to name an imported/exported package target. "
                "Local targets created directly inside package config files are not supported "
                "because they break static install/export paths.")
      endif()
      set(_zp_backend_imported_target "${ZP_BACKEND_IMPORTED_TARGET}")
    else()
      if("${ZP_BACKEND_SOURCE_FILES}" STREQUAL "")
        message(FATAL_ERROR
                "${kind} from ${_zp_backend_descriptor_file} must define either "
                "ZP_BACKEND_IMPORTED_TARGET or ZP_BACKEND_SOURCE_FILES")
      endif()
      set(_zp_backend_source_files "${ZP_BACKEND_SOURCE_FILES}")
      set(_zp_backend_include_dirs "${ZP_BACKEND_INCLUDE_DIRS}")
      set(_zp_backend_compile_options "${ZP_BACKEND_COMPILE_OPTIONS}")
      set(_zp_backend_link_libraries "${ZP_BACKEND_LINK_LIBRARIES}")
    endif()

    set("${prefix}_IMPORTED_TARGET" "${_zp_backend_imported_target}" PARENT_SCOPE)
    set("${prefix}_SOURCE_FILES" "${_zp_backend_source_files}" PARENT_SCOPE)
    set("${prefix}_INCLUDE_DIRS" "${_zp_backend_include_dirs}" PARENT_SCOPE)
    set("${prefix}_COMPILE_DEFINITIONS" "${_zp_backend_compile_definitions}" PARENT_SCOPE)
    set("${prefix}_COMPILE_OPTIONS" "${_zp_backend_compile_options}" PARENT_SCOPE)
    set("${prefix}_LINK_LIBRARIES" "${_zp_backend_link_libraries}" PARENT_SCOPE)
    set("${prefix}_COMPATIBLE_SYSTEM_LAYERS" "${_zp_backend_compatible_system_layers}" PARENT_SCOPE)
  else()
    set("${prefix}_IMPORTED_TARGET" "" PARENT_SCOPE)
    set("${prefix}_SOURCE_FILES" "" PARENT_SCOPE)
    set("${prefix}_INCLUDE_DIRS" "" PARENT_SCOPE)
    set("${prefix}_COMPILE_DEFINITIONS" "" PARENT_SCOPE)
    set("${prefix}_COMPILE_OPTIONS" "" PARENT_SCOPE)
    set("${prefix}_LINK_LIBRARIES" "" PARENT_SCOPE)
    set("${prefix}_COMPATIBLE_SYSTEM_LAYERS" "" PARENT_SCOPE)
  endif()
endfunction()

function(zp_validate_backend_selection type name compatible_system_layers system_layer)
  if("${name}" STREQUAL "")
    return()
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

set(ZP_TCP_BACKEND_DIRS "")
set(ZP_UDP_BACKEND_DIRS "")
set(ZP_BT_BACKEND_DIRS "")
set(ZP_SERIAL_BACKEND_DIRS "")
