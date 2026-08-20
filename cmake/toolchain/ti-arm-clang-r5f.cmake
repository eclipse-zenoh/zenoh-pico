# CMake toolchain file for TI ARM Clang targeting Cortex-R5F (tiarmclang)
#
# Required environment/CMake variables:
#   CGT_TI_ARM_CLANG_PATH — root of the TI ARM Clang installation
#                            e.g. /opt/ti/ti-cgt-armllvm_3.2.2.LTS
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/ti-arm-clang-r5f.cmake \
#         -DZP_PLATFORM=ti_am67a ...

# ---- Resolve toolchain path -----------------------------------------------
# Resolution order:
#   1. Already in the CMake cache  (set on a previous cmake run or by the caller)
#   2. Passed as -DCGT_TI_ARM_CLANG_PATH=... on the current cmake invocation
#   3. Set in the environment as CGT_TI_ARM_CLANG_PATH
#   4. Auto-detected from well-known /opt/ti install locations
#
# The resolved value is stored in the CACHE so CMake's internal try_compile
# sub-projects (which re-process this toolchain file in a fresh CMake instance)
# inherit it automatically via the CACHE file written by the parent.
if(NOT DEFINED CACHE{CGT_TI_ARM_CLANG_PATH})
    if(DEFINED CGT_TI_ARM_CLANG_PATH AND NOT CGT_TI_ARM_CLANG_PATH STREQUAL "")
        set(CGT_TI_ARM_CLANG_PATH "${CGT_TI_ARM_CLANG_PATH}" CACHE PATH
            "Root of TI ARM Clang installation" FORCE)
    elseif(DEFINED ENV{CGT_TI_ARM_CLANG_PATH} AND NOT "$ENV{CGT_TI_ARM_CLANG_PATH}" STREQUAL "")
        set(CGT_TI_ARM_CLANG_PATH "$ENV{CGT_TI_ARM_CLANG_PATH}" CACHE PATH
            "Root of TI ARM Clang installation" FORCE)
    else()
        # Auto-detect from common TI install locations
        foreach(_ti_dir
            "/opt/ti/ti-cgt-armllvm_3.2.2.LTS"
            "/opt/ti/ti-cgt-armllvm_3.2.1.LTS"
            "/opt/ti/ti-cgt-armllvm_3.2.0.LTS"
            "/opt/ti/ti-cgt-armllvm_3.1.0.LTS")
            if(EXISTS "${_ti_dir}/bin/tiarmclang")
                set(CGT_TI_ARM_CLANG_PATH "${_ti_dir}" CACHE PATH
                    "Root of TI ARM Clang installation (auto-detected)" FORCE)
                break()
            endif()
        endforeach()
        if(NOT DEFINED CACHE{CGT_TI_ARM_CLANG_PATH})
            message(FATAL_ERROR
                "CGT_TI_ARM_CLANG_PATH is not set and tiarmclang was not found in /opt/ti/.\n"
                "Export it as an environment variable or pass -DCGT_TI_ARM_CLANG_PATH=<path>.\n"
                "Example: cmake ... -DCGT_TI_ARM_CLANG_PATH=/opt/ti/ti-cgt-armllvm_3.2.2.LTS")
        endif()
    endif()
endif()

# ---- System identity -------------------------------------------------------
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# ---- Compiler / archiver ---------------------------------------------------
set(CMAKE_C_COMPILER   "${CGT_TI_ARM_CLANG_PATH}/bin/tiarmclang" CACHE PATH "TI ARM Clang C compiler")
set(CMAKE_CXX_COMPILER "${CGT_TI_ARM_CLANG_PATH}/bin/tiarmclang" CACHE PATH "TI ARM Clang C++ compiler")
set(CMAKE_AR           "${CGT_TI_ARM_CLANG_PATH}/bin/tiarmar"    CACHE PATH "TI ARM archiver")

# tiarmar does not accept a bare filename as a ranlib invocation.
# Use 'rcs' (replace + create + symbol index) so no separate ranlib step is needed.
set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> rcs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_APPEND   "<CMAKE_AR> rs  <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH   "")
set(CMAKE_RANLIB             "" CACHE PATH "" FORCE)

# ---- Cortex-R5F CPU flags --------------------------------------------------
set(_TI_CPU_FLAGS "--target=arm-ti-none-eabi -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -mthumb")

set(CMAKE_C_FLAGS_INIT   "${_TI_CPU_FLAGS}" CACHE STRING "Initial C flags")
set(CMAKE_CXX_FLAGS_INIT "${_TI_CPU_FLAGS}" CACHE STRING "Initial C++ flags")

# ---- Build type flags ------------------------------------------------------
set(CMAKE_C_FLAGS_DEBUG          "-g -O0" CACHE STRING "Debug C flags")
set(CMAKE_C_FLAGS_RELEASE        "-O2 -DNDEBUG" CACHE STRING "Release C flags")
set(CMAKE_C_FLAGS_MINSIZEREL     "-Oz -DNDEBUG" CACHE STRING "MinSizeRel C flags")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE STRING "RelWithDebInfo C flags")

# ---- Linker ----------------------------------------------------------------
# Static library: no linker needed for the library target itself.
# For executable targets the user must supply a linker command file.
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_TI_CPU_FLAGS}" CACHE STRING "Linker flags")

# Prevent CMake from testing the compiler with a hosted executable
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ---- Find behaviour for cross-compilation ----------------------------------
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
