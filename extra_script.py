Import('env')
from os.path import join, realpath

src_filter = []
cppdefines = []

framework = env.get("PIOFRAMEWORK")[0]
if framework == 'zephyr':
  src_filter=["+<*>",
              "-<tests/>",
              "-<example/>",
              "-<system/unix/>",
              "-<system/esp32/>"]
  cppdefines=["ZENOH_ZEPHYR"]

elif framework == 'arduino':
  platform = env.get("PIOPLATFORM")
  if platform == 'espressif32':
    src_filter=["+<*>",
                "-<tests/>",
                "-<example/>",
                "-<system/unix/>",
                "-<system/zephyr/>"]
    cppdefines=["ZENOH_ESP32"]


env.Append(SRC_FILTER=src_filter)
env.Append(CPPDEFINES=cppdefines)

# pass flags to a global build environment (for all libraries, etc)
global_env = DefaultEnvironment()
global_env.Append(CPPDEFINES=cppdefines)
