#
# Copyright (c) 2022 ZettaScale Technology
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
# which is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
#
# Contributors:
#   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
#
import os
import subprocess

Import('env', 'projenv')

SRC_FILTER = []
CPPDEFINES = []
ZP_PLATFORM = None

BASE_SRC_FILTER = [
    "+<*>",
    "-<tests/>",
    "-<example/>",
    "-<system/>",
    "+<system/common/>",
    "+<system/socket/>",
]


def _platform_src_filter(system_dir):
    return BASE_SRC_FILTER + [f"+<{system_dir}/>"]

FRAMEWORK = env.get("PIOFRAMEWORK")[0]
PLATFORM = env.get("PIOPLATFORM")
BOARD = env.get("PIOENV")
ZENOH_GENERIC = env.get("ZENOH_GENERIC", "0")
if ZENOH_GENERIC == "1":
    FRAMEWORK = 'generic'
    PLATFORM = 'generic'
    BOARD = 'generic'

if FRAMEWORK == 'zephyr':
    SRC_FILTER = _platform_src_filter("system/zephyr")
    ZP_PLATFORM = "zephyr"
    CPPDEFINES = [
        "ZENOH_ZEPHYR",
    ]

elif FRAMEWORK == 'arduino':
    PLATFORM = env.get("PIOPLATFORM")
    if PLATFORM == 'espressif32':
        SRC_FILTER = _platform_src_filter("system/arduino/esp32")
        ZP_PLATFORM = "arduino_esp32"
        CPPDEFINES = [
            "ZENOH_ARDUINO_ESP32",
            "ZENOH_COMPILER_GCC",
            "ZENOH_C_STANDARD=99",
        ]
    if PLATFORM == 'ststm32':
        BOARD = env.get("PIOENV")
        if BOARD == 'opencr':
            SRC_FILTER = _platform_src_filter("system/arduino/opencr")
            ZP_PLATFORM = "opencr"
            CPPDEFINES = [
                "ZENOH_ARDUINO_OPENCR",
                "ZENOH_C_STANDARD=99",
                "Z_FEATURE_MULTI_THREAD=0",
            ]

elif FRAMEWORK == 'espidf':
    SRC_FILTER = _platform_src_filter("system/espidf")
    ZP_PLATFORM = "espidf"
    CPPDEFINES = [
        "ZENOH_ESPIDF",
    ]

elif FRAMEWORK == 'mbed':
    SRC_FILTER = _platform_src_filter("system/mbed")
    ZP_PLATFORM = "mbed"
    CPPDEFINES = [
        "ZENOH_MBED",
        "ZENOH_C_STANDARD=99",
    ]
elif FRAMEWORK == 'generic':
    SRC_FILTER = BASE_SRC_FILTER
    CPPDEFINES = ["ZENOH_GENERIC"]

env.Append(SRC_FILTER=SRC_FILTER)
env.Append(CPPDEFINES=CPPDEFINES)

# pass flags to the main project environment
projenv.Append(CPPDEFINES=CPPDEFINES)

# pass flags to a global build environment (for all libraries, etc)
global_env = DefaultEnvironment()
global_env.Append(CPPDEFINES=CPPDEFINES)

# Run CMake for zenoh-pico

# Default embedded buffer values
default_args = ["-DFRAG_MAX_SIZE=4096", "-DBATCH_UNICAST_SIZE=2048", "-DBATCH_MULTICAST_SIZE=2048"]

# Get the additional CMake arguments from the environment
cmake_extra_args = env.BoardConfig().get("build.cmake_extra_args", "")
cmake_extra_args_list = cmake_extra_args.split()


def _has_cmake_arg(name):
    prefix = f"{name}="
    return any(arg.startswith(prefix) for arg in cmake_extra_args_list)


if (
    ZP_PLATFORM is not None
    and not _has_cmake_arg("-DZP_PLATFORM")
):
    cmake_extra_args_list.append(f"-DZP_PLATFORM={ZP_PLATFORM}")

# Add default value if needed
for default in default_args:
    arg_name = default.split('=')[0]
    if not any(arg.startswith(arg_name) for arg in cmake_extra_args_list):
        cmake_extra_args_list.append(default)

# Define the source and binary directories
source_dir = os.getcwd()
build_dir = os.path.join(source_dir, "build")
os.makedirs(build_dir, exist_ok=True)
generated_include_dir = os.path.join(build_dir, "include")
for build_env in (env, projenv, global_env):  # pylint: disable=undefined-variable
    build_env.Prepend(CPPPATH=[generated_include_dir])
    build_env.Prepend(CCFLAGS=[f"-I{generated_include_dir}"])
# Run the CMake command with the source and binary directories
print("Run command: cmake", source_dir, cmake_extra_args_list)
subprocess.run(["cmake", source_dir] + cmake_extra_args_list, cwd=build_dir, check=True)
