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

Import('env', 'projenv')
from os.path import join, realpath

src_filter = []
cppdefines = []

framework = env.get("PIOFRAMEWORK")[0]
if framework == 'zephyr':
    src_filter=["+<*>",
                "-<tests/>",
                "-<example/>",
                "-<system/espidf/>",
                "-<system/mbed/>",
                "-<system/unix/>",
                "-<system/arduino/>"]
    cppdefines=["ZENOH_ZEPHYR"]

elif framework == 'arduino':
    platform = env.get("PIOPLATFORM")
    if platform == 'espressif32':
        src_filter=["+<*>",
                    "-<tests/>",
                    "-<example/>",
                    "-<system/espidf>",
                    "-<system/mbed/>",
                    "-<system/arduino/opencr>",
                    "-<system/unix/>",
                    "-<system/zephyr/>"]
        cppdefines=["ZENOH_ARDUINO_ESP32", "ZENOH_C_STANDARD=99"]
    if platform == 'ststm32':
        board = env.get("PIOENV")
        if board == 'opencr':
            src_filter=["+<*>",
                        "-<tests/>",
                        "-<example/>",
                        "-<system/espidf>",
                        "-<system/mbed/>",
                        "-<system/arduino/esp32>",
                        "-<system/unix/>",
                        "-<system/zephyr/>"]
            cppdefines=["ZENOH_ARDUINO_OPENCR", "ZENOH_C_STANDARD=99", "Z_MULTI_THREAD=0"]

elif framework == 'espidf':
    src_filter=["+<*>",
                "-<tests/>",
                "-<example/>",
                "-<system/mbed/>",
                "-<system/unix/>",
                "-<system/arduino/>",
                "-<system/zephyr/>"]
    cppdefines=["ZENOH_ESPIDF"]

elif framework == 'mbed':
    src_filter=["+<*>",
                "-<tests/>",
                "-<example/>",
                "-<system/espidf/>",
                "-<system/unix/>",
                "-<system/arduino/>",
                "-<system/zephyr/>"]
    cppdefines=["ZENOH_MBED", "ZENOH_C_STANDARD=99"]

env.Append(SRC_FILTER=src_filter)
env.Append(CPPDEFINES=cppdefines)

# pass flags to the main project environment
projenv.Append(CPPDEFINES=cppdefines)

# pass flags to a global build environment (for all libraries, etc)
global_env = DefaultEnvironment()
global_env.Append(CPPDEFINES=cppdefines)
