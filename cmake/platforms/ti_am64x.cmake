# CMake platform profile for TI AM64x Cortex-R5F (FreeRTOS + lwIP, TI MCU+ SDK)
#
# Required environment/CMake variables:
#   MCU_PLUS_SDK_PATH   — root of the TI MCU+ SDK AM64x installation
#   CGT_TI_ARM_CLANG_PATH — root of the TI ARM Clang toolchain (optional here,
#                           used by cmake/toolchain/ti-arm-clang-r5f.cmake)
#
# Typical invocation:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/ti-arm-clang-r5f.cmake \
#         -DZP_PLATFORM=ti_am64x \
#         -DBUILD_SHARED_LIBS=OFF \
#         -B build/ti_am64x -S .

# ---- Resolve MCU+ SDK path ------------------------------------------------
if(NOT DEFINED MCU_PLUS_SDK_PATH OR MCU_PLUS_SDK_PATH STREQUAL "")
    if(DEFINED ENV{MCU_PLUS_SDK_PATH} AND NOT "$ENV{MCU_PLUS_SDK_PATH}" STREQUAL "")
        set(MCU_PLUS_SDK_PATH "$ENV{MCU_PLUS_SDK_PATH}")
    else()
        foreach(_sdk_dir
            "/opt/ti/mcu_plus_sdk_am64x_12_00_00_27"
            "/opt/ti/mcu_plus_sdk_am64x_12_00_00_00"
            "/opt/ti/mcu_plus_sdk_am64x_11_02_00_00")
            if(EXISTS "${_sdk_dir}/source")
                set(MCU_PLUS_SDK_PATH "${_sdk_dir}")
                break()
            endif()
        endforeach()
        if(NOT DEFINED MCU_PLUS_SDK_PATH OR MCU_PLUS_SDK_PATH STREQUAL "")
            message(FATAL_ERROR
                "MCU_PLUS_SDK_PATH is not set and was not found in /opt/ti/.\n"
                "Export it as an environment variable or pass -DMCU_PLUS_SDK_PATH=<path>.\n"
                "Example: cmake ... -DMCU_PLUS_SDK_PATH=/opt/ti/mcu_plus_sdk_am64x_12_00_00_27")
        endif()
    endif()
endif()

# ---- Platform identifiers --------------------------------------------------
# ZP_PLATFORM_SYSTEM_LAYER is informational; the actual dispatch uses ZENOH_TI_AM64X.
set(ZP_PLATFORM_SYSTEM_LAYER ti_am64x)
set(ZP_PLATFORM_COMPILE_DEFINITIONS
    ZENOH_TI_AM64X
    SOC_AM64X
    OS_FREERTOS)

# ---- SDK include directories -----------------------------------------------
set(ZP_PLATFORM_INCLUDE_DIRS
    "${MCU_PLUS_SDK_PATH}/source"
    "${MCU_PLUS_SDK_PATH}/source/kernel/freertos/FreeRTOS-Kernel/include"
    "${MCU_PLUS_SDK_PATH}/source/kernel/freertos/portable/TI_ARM_CLANG/ARM_CR5F"
    "${MCU_PLUS_SDK_PATH}/source/kernel/freertos/config/am64x/r5f"
    "${MCU_PLUS_SDK_PATH}/source/networking/lwip/lwip-stack/src/include"
    "${MCU_PLUS_SDK_PATH}/source/networking/lwip/lwip-port/include"
    "${MCU_PLUS_SDK_PATH}/source/networking/lwip/lwip-port/freertos/include"
    "${MCU_PLUS_SDK_PATH}/source/networking/lwip/lwip-config/am64x")

# ---- Source files ----------------------------------------------------------
set(ZP_PLATFORM_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/system/ti_am64x/system.c"
    "${PROJECT_SOURCE_DIR}/src/system/ti_am64x/mem_pool.c"
    "${PROJECT_SOURCE_DIR}/src/system/socket/lwip.c"
    "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_lwip.c")

if(ZP_UDP_MULTICAST_ENABLED)
    list(APPEND ZP_PLATFORM_SOURCE_FILES
         "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip.c"
         "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip_common.c")
endif()

# Disable TCP/TLS/WS — only UDP multicast is supported on this bare-metal target.
# tcp_lwip.c requires <unistd.h> which is not available in tiarmclang sysroot.
set(Z_FEATURE_LINK_TCP 0 CACHE STRING "TCP disabled on ti_am64x (no unistd.h)" FORCE)
set(Z_FEATURE_LINK_TLS 0 CACHE STRING "TLS disabled on ti_am64x" FORCE)
set(Z_FEATURE_LINK_WS  0 CACHE STRING "WS disabled on ti_am64x" FORCE)

# Bare-metal target: static library only.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Static only on ti_am64x" FORCE)

# Disable the automatic pthread search — FreeRTOS supplies its own threading
set(CHECK_THREADS OFF)
