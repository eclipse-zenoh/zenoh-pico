function(zp_resolve_system_layer descriptor_label)
  if("${descriptor_label}" STREQUAL "")
    set(descriptor_label "platform descriptor")
  endif()

  set(_zp_system_imported_target "")
  set(_zp_system_source_files "")
  set(_zp_system_include_dirs "")
  set(_zp_system_compile_definitions "${ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS}")
  set(_zp_system_platform_header "${ZP_PLATFORM_SYSTEM_PLATFORM_HEADER}")
  set(_zp_system_has_builtin_component OFF)
  foreach(_zp_component_var IN ITEMS SOURCE_FILES INCLUDE_DIRS)
    if(NOT "${ZP_PLATFORM_SYSTEM_${_zp_component_var}}" STREQUAL "")
      set(_zp_system_has_builtin_component ON)
      break()
    endif()
  endforeach()
  if(NOT "${ZP_PLATFORM_SYSTEM_IMPORTED_TARGET}" STREQUAL "" AND _zp_system_has_builtin_component)
    message(FATAL_ERROR
            "System layer from ${descriptor_label} may not define both "
            "ZP_PLATFORM_SYSTEM_IMPORTED_TARGET and built-in source/include variables")
  endif()

  if(NOT "${ZP_PLATFORM_SYSTEM_IMPORTED_TARGET}" STREQUAL "")
    if(NOT TARGET "${ZP_PLATFORM_SYSTEM_IMPORTED_TARGET}")
      message(FATAL_ERROR
              "System layer from ${descriptor_label} references missing target "
              "${ZP_PLATFORM_SYSTEM_IMPORTED_TARGET}")
    endif()
    get_property(_zp_is_imported TARGET "${ZP_PLATFORM_SYSTEM_IMPORTED_TARGET}" PROPERTY IMPORTED)
    if(NOT _zp_is_imported)
      message(FATAL_ERROR
              "System layer from ${descriptor_label} requires "
              "ZP_PLATFORM_SYSTEM_IMPORTED_TARGET to name an imported/exported package target. "
              "Local targets created directly inside package config files are not supported "
              "because they break static install/export paths.")
    endif()
    set(_zp_system_imported_target "${ZP_PLATFORM_SYSTEM_IMPORTED_TARGET}")
  else()
    set(_zp_system_source_files "${ZP_PLATFORM_SYSTEM_SOURCE_FILES}")
    set(_zp_system_include_dirs "${ZP_PLATFORM_SYSTEM_INCLUDE_DIRS}")
  endif()

  set(ZP_SYSTEM_IMPORTED_TARGET "${_zp_system_imported_target}" PARENT_SCOPE)
  set(ZP_SYSTEM_SOURCE_FILES "${_zp_system_source_files}" PARENT_SCOPE)
  set(ZP_SYSTEM_INCLUDE_DIRS "${_zp_system_include_dirs}" PARENT_SCOPE)
  set(ZP_SYSTEM_COMPILE_DEFINITIONS "${_zp_system_compile_definitions}" PARENT_SCOPE)
  set(ZP_SYSTEM_PLATFORM_HEADER "${_zp_system_platform_header}" PARENT_SCOPE)
  set(ZP_SYSTEM_COMPILE_OPTIONS "" PARENT_SCOPE)
  set(ZP_SYSTEM_LINK_LIBRARIES "" PARENT_SCOPE)
endfunction()
