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
    cppdefines=["ZENOH_ARDUINO_ESP32"]
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
      cppdefines=["ZENOH_ARDUINO_OPENCR"]

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
  cppdefines=["ZENOH_MBED"]

env.Append(SRC_FILTER=src_filter)
env.Append(CPPDEFINES=cppdefines)

# pass flags to the main project environment
projenv.Append(CPPDEFINES=cppdefines)

# pass flags to a global build environment (for all libraries, etc)
global_env = DefaultEnvironment()
global_env.Append(CPPDEFINES=cppdefines)
