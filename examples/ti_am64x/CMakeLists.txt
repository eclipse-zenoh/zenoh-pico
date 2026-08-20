# CMake build for zenoh-pico T3 Gemstone O1 / TI AM64x examples
#
# Included by examples/CMakeLists.txt when ZP_PLATFORM == "ti_am64x".
# MCU_PLUS_SDK_PATH is already resolved by cmake/platforms/ti_am64x.cmake.
#
# SysConfig files are generated automatically when SYSCONFIG_CLI is found or
# set on the command line.  If not found, pre-generated files in syscfg/ are
# used (e.g. committed by the developer after running SysConfig manually).
#
# Explicit override:
#   cmake ... -DSYSCONFIG_CLI=/opt/ti/sysconfig_1.28.0/sysconfig_cli.sh

# ---- IPC mode option --------------------------------------------------------
#
# -DZENOH_TI_AM64X_IPC=ON   : Use RPMsg IPC transport instead of CPSW Ethernet.
#                              Ethernet frames travel over the VRING shared-memory
#                              channel to the Linux A53 core.  No CPSW hardware or
#                              UDMA is required.
#
# When ON:
#   - examples link against lwip + lwip_contrib only (no enet_cpsw / lwipif_cpsw)
#   - zenoh_net_init_rpmsg.c replaces zenoh_net_init.c
#   - rpmsg_lwip_netif.c provides the custom lwIP netif
#   - SysConfig is generated from example_ipc.syscfg (no CPSW, only IPC module)
#   - A dedicated IPC linker file is used (linker/r5fss0-0_ipc.cmd.ld)
#
# On Linux, load the firmware via remoteproc, then:
#   sudo insmod rpmsg_net.ko
#   sudo ip addr add 192.168.200.2/30 dev rpmsg0
#   sudo ip link set rpmsg0 up
#
option(ZENOH_TI_AM64X_IPC
    "Use RPMsg IPC transport instead of CPSW Ethernet on TI AM64x" OFF)

# ---- SDK pre-built library targets ------------------------------------------
set(_SDK_SRC "${MCU_PLUS_SDK_PATH}/source")

macro(_import_sdk_lib _alias _path)
    add_library(${_alias} STATIC IMPORTED GLOBAL)
    set_target_properties(${_alias} PROPERTIES IMPORTED_LOCATION "${_path}")
endmacro()

_import_sdk_lib(ti_am64x_sdk::freertos
    "${_SDK_SRC}/kernel/freertos/lib/freertos.am64x.r5f.ti-arm-clang.release.lib")
_import_sdk_lib(ti_am64x_sdk::drivers
    "${_SDK_SRC}/drivers/lib/drivers.am64x.r5f.ti-arm-clang.freertos.release.lib")
_import_sdk_lib(ti_am64x_sdk::board
    "${_SDK_SRC}/board/lib/board.am64x.r5f.ti-arm-clang.freertos.release.lib")
_import_sdk_lib(ti_am64x_sdk::enet_cpsw
    "${_SDK_SRC}/networking/enet/lib/enet-cpsw.am64x.r5f.ti-arm-clang.freertos.release.lib")
_import_sdk_lib(ti_am64x_sdk::lwipif_cpsw
    "${_SDK_SRC}/networking/enet/lib/lwipif-cpsw-freertos.am64x.r5f.ti-arm-clang.release.lib")
_import_sdk_lib(ti_am64x_sdk::lwip
    "${_SDK_SRC}/networking/lwip/lib/lwip-freertos.am64x.r5f.ti-arm-clang.release.lib")
_import_sdk_lib(ti_am64x_sdk::lwip_contrib
    "${_SDK_SRC}/networking/lwip/lib/lwip-contrib-freertos.am64x.r5f.ti-arm-clang.release.lib")

# ---- SysConfig setup --------------------------------------------------------
if(ZENOH_TI_AM64X_IPC)
    set(_SYSCFG_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/syscfg_ipc")
    set(_SYSCFG_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/example_ipc.syscfg")
else()
    set(_SYSCFG_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/syscfg")
    set(_SYSCFG_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/example.syscfg")
endif()

# Canonical set of files SysConfig generates — identical for both IPC and CPSW modes.
# In IPC mode the ti_enet_* files are generated as empty stubs (no includes, no code)
# so they compile safely without enet headers and produce empty object files.
# The IPC init code lives in ti_drivers_config.c (VRING memory, RPMessage resource table).
set(_SYSCFG_ALL_SRCS
    "${_SYSCFG_DIR}/ti_enet_lwipif.c"
    "${_SYSCFG_DIR}/ti_enet_config.c"
    "${_SYSCFG_DIR}/ti_enet_open_close.c"
    "${_SYSCFG_DIR}/ti_enet_init.c"
    "${_SYSCFG_DIR}/ti_enet_dma_init.c"
    "${_SYSCFG_DIR}/ti_enet_soc.c"
    "${_SYSCFG_DIR}/ti_drivers_config.c"
    "${_SYSCFG_DIR}/ti_drivers_open_close.c"
    "${_SYSCFG_DIR}/ti_board_config.c"
    "${_SYSCFG_DIR}/ti_board_open_close.c"
    "${_SYSCFG_DIR}/ti_dpl_config.c"
    "${_SYSCFG_DIR}/ti_pinmux_config.c"
    "${_SYSCFG_DIR}/ti_power_clock_config.c"
)
set(_SYSCFG_ALL_HDRS
    "${_SYSCFG_DIR}/ti_enet_lwipif.h"
    "${_SYSCFG_DIR}/ti_enet_config.h"
    "${_SYSCFG_DIR}/ti_enet_open_close.h"
    "${_SYSCFG_DIR}/ti_enet_dma_init.h"
    "${_SYSCFG_DIR}/ti_dpl_config.h"
    "${_SYSCFG_DIR}/ti_drivers_config.h"
    "${_SYSCFG_DIR}/ti_drivers_open_close.h"
    "${_SYSCFG_DIR}/ti_board_config.h"
    "${_SYSCFG_DIR}/ti_board_open_close.h"
)
set(_SYSCFG_ALL_OUTPUTS ${_SYSCFG_ALL_HDRS} ${_SYSCFG_ALL_SRCS})

# ---- Locate SysConfig CLI ---------------------------------------------------
find_program(SYSCONFIG_CLI
    NAMES sysconfig_cli.sh
    HINTS
        ENV SYSCONFIG_PATH
        /opt/ti/sysconfig_1.28.0
        /opt/ti/sysconfig_1.26.2
        /opt/ti/sysconfig_1.26.1
        /opt/ti/sysconfig_1.26.0
        /opt/ti/sysconfig
    PATH_SUFFIXES "."
    DOC "Path to TI SysConfig CLI (sysconfig_cli.sh). Set -DSYSCONFIG_CLI=... to override."
)

set(_SYSCFG_PRODUCT "${MCU_PLUS_SDK_PATH}/.metadata/product.json")

if(SYSCONFIG_CLI AND EXISTS "${_SYSCFG_PRODUCT}")
    message(STATUS "SysConfig CLI : ${SYSCONFIG_CLI}")
    message(STATUS "SysConfig will generate files into ${_SYSCFG_DIR}")

    if(ZENOH_TI_AM64X_IPC)
        set(_IPC_PATCH_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/patch_ti_ipc_config.py")
        add_custom_command(
            OUTPUT  ${_SYSCFG_ALL_OUTPUTS}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_SYSCFG_DIR}"
            COMMAND "${SYSCONFIG_CLI}"
                    -s "${_SYSCFG_PRODUCT}"
                    -o "${_SYSCFG_DIR}"
                    "${_SYSCFG_SCRIPT}"
            # Patch the generated ti_drivers_config.c: SysConfig emits empty
            # gIpcSharedMem[] subscripts when memory_configurator is absent.
            # Replace with 0U — safe for Linux-IPC-only builds.
            COMMAND ${CMAKE_COMMAND} -E env python3
                    "${_IPC_PATCH_SCRIPT}"
                    "${_SYSCFG_DIR}/ti_drivers_config.c"
            DEPENDS "${_SYSCFG_SCRIPT}" "${_IPC_PATCH_SCRIPT}"
            COMMENT "Running SysConfig CLI for ti_am64x IPC examples"
            VERBATIM
        )
    else()
        set(_PATCH_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/patch_ti_enet_init.py")
        add_custom_command(
            OUTPUT  ${_SYSCFG_ALL_OUTPUTS}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_SYSCFG_DIR}"
            COMMAND "${SYSCONFIG_CLI}"
                    -s "${_SYSCFG_PRODUCT}"
                    -o "${_SYSCFG_DIR}"
                    "${_SYSCFG_SCRIPT}"
            # Patch the generated ti_enet_init.c: const struct in static initializer
            # is not valid C (tiarmclang enforces this). Move the assignment to runtime.
            COMMAND ${CMAKE_COMMAND} -E env python3
                    "${_PATCH_SCRIPT}"
                    "${_SYSCFG_DIR}/ti_enet_init.c"
            DEPENDS "${_SYSCFG_SCRIPT}" "${_PATCH_SCRIPT}"
            COMMENT "Running SysConfig CLI for ti_am64x examples"
            VERBATIM
        )
    endif()

    set(_SYSCFG_SRCS ${_SYSCFG_ALL_SRCS})
    add_custom_target(ti_am64x_syscfg DEPENDS ${_SYSCFG_ALL_OUTPUTS})
else()
    if(NOT SYSCONFIG_CLI)
        message(WARNING
            "\n[ti_am64x] SysConfig CLI not found.\n"
            "  Install SysConfig >= 1.21.2 and set:\n"
            "    -DSYSCONFIG_CLI=/opt/ti/sysconfig_<ver>/sysconfig_cli.sh\n"
            "  or set env var SYSCONFIG_PATH=/opt/ti/sysconfig_<ver>\n"
            "  Falling back to pre-generated files in '${_SYSCFG_DIR}'.\n")
    endif()

    # Collect whichever generated files already exist (manual run)
    set(_SYSCFG_SRCS "")
    foreach(_sc IN LISTS _SYSCFG_ALL_SRCS)
        if(EXISTS "${_sc}")
            list(APPEND _SYSCFG_SRCS "${_sc}")
        endif()
    endforeach()

    if(NOT EXISTS "${_SYSCFG_DIR}/ti_enet_lwipif.h")
        message(WARNING
            "\n[ti_am64x] SysConfig-generated headers not found in '${_SYSCFG_DIR}'.\n"
            "  Examples will not compile until SysConfig is run.\n"
            "  See examples/ti_am64x/README.md for instructions.\n")
    endif()

    add_custom_target(ti_am64x_syscfg)  # empty placeholder so dependencies resolve
endif()

# ---- Linker command files ---------------------------------------------------
set(_LINKER_CMD_CPSW "${CMAKE_CURRENT_SOURCE_DIR}/linker/r5fss0-0.cmd.ld")
set(_LINKER_CMD_IPC  "${CMAKE_CURRENT_SOURCE_DIR}/linker/r5fss0-0_ipc.cmd.ld")

if(ZENOH_TI_AM64X_IPC)
    set(_LINKER_CMD "${_LINKER_CMD_IPC}")
else()
    set(_LINKER_CMD "${_LINKER_CMD_CPSW}")
endif()

# ---- Helper: create one example executable ----------------------------------
#
# add_ti_am64x_example(<name> <main_src>)
#
function(add_ti_am64x_example _name _main_src)
    if(ZENOH_TI_AM64X_IPC)
        # IPC mode: use RPMsg netif, no CPSW stub
        set(_net_srcs
            ${CMAKE_CURRENT_SOURCE_DIR}/common/zenoh_net_init_rpmsg.c
            ${CMAKE_CURRENT_SOURCE_DIR}/common/rpmsg_lwip_netif.c
        )
    else()
        # Default CPSW Ethernet mode
        set(_net_srcs
            ${CMAKE_CURRENT_SOURCE_DIR}/common/zenoh_net_init.c
            ${CMAKE_CURRENT_SOURCE_DIR}/common/enet_board_stub.c
        )
    endif()

    add_executable(${_name}
        ${_main_src}
        ${_net_srcs}
        ${_SYSCFG_SRCS}
    )

    add_dependencies(${_name} ti_am64x_syscfg)

    set_property(TARGET ${_name} PROPERTY C_STANDARD 11)
    set_property(TARGET ${_name} PROPERTY SUFFIX ".out")

    if(ZENOH_TI_AM64X_IPC)
        target_compile_definitions(${_name} PRIVATE
            SOC_AM64X
            OS_FREERTOS
            ZENOH_TI_AM64X
            ZENOH_TI_AM64X_IPC
        )
    else()
        target_compile_definitions(${_name} PRIVATE
            SOC_AM64X
            OS_FREERTOS
            ZENOH_TI_AM64X
        )
    endif()

    if(ZENOH_TI_AM64X_IPC)
        # IPC mode: only lwIP stack, no CPSW enet headers
        target_include_directories(${_name} PRIVATE
            ${ZP_PLATFORM_INCLUDE_DIRS}
            ${_SYSCFG_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/common
        )
        target_link_libraries(${_name} PRIVATE
            zenohpico::lib
            ti_am64x_sdk::lwip
            ti_am64x_sdk::lwip_contrib
            ti_am64x_sdk::drivers
            ti_am64x_sdk::board
            ti_am64x_sdk::freertos
        )
    else()
        # CPSW Ethernet mode: extra enet/lwipif headers needed by SysConfig files
        set(_ENET_ROOT "${MCU_PLUS_SDK_PATH}/source/networking/enet")
        target_include_directories(${_name} PRIVATE
            ${ZP_PLATFORM_INCLUDE_DIRS}
            ${_SYSCFG_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            # lwip contrib headers (V3 utils variant used by AM64x SDK)
            "${MCU_PLUS_SDK_PATH}/source/networking/lwip/lwip-stack/contrib"
            # enet core flat includes: <enet.h>, <enet_cfg.h>
            "${_ENET_ROOT}/core/include"
            "${_ENET_ROOT}/core/include/core"
            "${_ENET_ROOT}/core/include/phy"
            # enet utils: <enet_board.h>, <enet_apputils.h>, etc.
            "${_ENET_ROOT}/core/utils/include"
            "${_ENET_ROOT}/core/utils/V3"
            # lwipif: "lwip2enet.h"
            "${_ENET_ROOT}/core/lwipif/inc"
            # enet core root: <include/core/...>, <priv/...>, <soc/...>, <src/...>
            "${_ENET_ROOT}/core"
            # enet hw_include: <networking/enet/hw_include/...>
            "${_ENET_ROOT}/hw_include"
            # ethphy rtos drivers
            "${MCU_PLUS_SDK_PATH}/source/board/ethphy/enet/rtos_drivers/include"
            # SoC-specific config: <enet_soc_cfg.h>
            "${_ENET_ROOT}/soc/k3/am64x_am243x"
            # enet root: <soc/k3/am64x_am243x/enet_soc.h>, <hw_include/...>
            "${_ENET_ROOT}"
        )
        target_link_libraries(${_name} PRIVATE
            zenohpico::lib
            ti_am64x_sdk::lwipif_cpsw
            ti_am64x_sdk::lwip
            ti_am64x_sdk::lwip_contrib
            ti_am64x_sdk::enet_cpsw
            ti_am64x_sdk::drivers
            ti_am64x_sdk::board
            ti_am64x_sdk::freertos
        )
    endif()

    target_link_options(${_name} PRIVATE
        -Wl,--diag_suppress=10063
        -Wl,--priority
        -Wl,--ram_model
        -Wl,--reread_libs
        ${_LINKER_CMD}
    )

    add_dependencies(examples ${_name})
endfunction()

# ---- Example targets --------------------------------------------------------
add_ti_am64x_example(z_pub     z_pub/main.c)
add_ti_am64x_example(z_sub     z_sub/main.c)
add_ti_am64x_example(z_pubsub  z_pubsub/main.c)
