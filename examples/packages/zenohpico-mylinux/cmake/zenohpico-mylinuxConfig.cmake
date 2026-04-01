include("${CMAKE_CURRENT_LIST_DIR}/zenohpicoMylinuxTargets.cmake")

if(COMMAND zp_add_platform_dir)
  zp_add_platform_dir("${CMAKE_CURRENT_LIST_DIR}/platforms")
endif()

list(APPEND ZP_SYSTEM_LAYER_DIRS "${CMAKE_CURRENT_LIST_DIR}/system")
list(APPEND ZP_SERIAL_BACKEND_DIRS "${CMAKE_CURRENT_LIST_DIR}/backends/serial")
