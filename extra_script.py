Import('env')
from os.path import join, realpath

env.Append(SRC_FILTER=["+<*>", "-<tests/>", "-<example/>", "-<system/unix/>"])
env.Append(CPPDEFINES=["ZENOH_ZEPHYR"])

# pass flags to a global build environment (for all libraries, etc)
global_env = DefaultEnvironment()
global_env.Append(
    CPPDEFINES=[
        "ZENOH_ZEPHYR",
    ]
)
