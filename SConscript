from building import *

cwd = GetCurrentDir()
src = []
CPPPATH = []

# Add include paths
CPPPATH += [cwd + '/include']

# Add source files
src += Glob('src/api/*.c')
src += Glob('src/collections/*.c')
src += Glob('src/link/*.c')
src += Glob('src/link/config/*.c')
src += Glob('src/link/multicast/*.c')
src += Glob('src/link/unicast/*.c')
src += Glob('src/net/*.c')
src += Glob('src/protocol/*.c')
src += Glob('src/protocol/codec/*.c')
src += Glob('src/protocol/definitions/*.c')
src += Glob('src/session/*.c')
src += Glob('src/system/common/*.c')
src += Glob('src/system/rtthread/*.c')
src += Glob('src/transport/*.c')
src += Glob('src/transport/common/*.c')
src += Glob('src/transport/multicast/*.c')
src += Glob('src/transport/raweth/*.c')
src += Glob('src/transport/unicast/*.c')
src += Glob('src/utils/*.c')

# Add RT-Thread specific defines
CPPDEFINES = ['ZENOH_RTTHREAD']

# Create library
group = DefineGroup('zenoh-pico', src, depend = [''], CPPPATH = CPPPATH, CPPDEFINES = CPPDEFINES)

Return('group')