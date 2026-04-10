include("${CMAKE_CURRENT_LIST_DIR}/zenohpicoMylinuxTargets.cmake")

if(COMMAND zp_add_platform_dir)
  zp_add_platform_dir("${CMAKE_CURRENT_LIST_DIR}/platforms")
endif()
