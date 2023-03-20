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
# ZettaScale Zenoh Team, <zenoh@zettascale.tech>
#

file(GLOB Sources "*.c"
  "api/*.c"
  "net/*.c"
  "collections/*.c"
  "link/*.c"
  "link/config/*.c"
  "link/unicast/*.c"
  "link/multicast/*.c"
  "protocol/*.c"
  "protocol/keyexpr/*.c"
  "session/*.c"
  "system/*.c"
  "transport/*.c"
  "transport/common/*.c"
  "transport/unicast/*.c"
  "transport/unicast/link/*.c"
  "transport/unicast/link/task/*.c"
  "transport/multicast/*.c"
  "transport/multicast/link/*.c"
  "transport/multicast/link/task/*.c"
  "utils/*.c"
)

if(POSIX_COMPATIBLE OR
   CMAKE_SYSTEM_NAME MATCHES "Linux" OR
   CMAKE_SYSTEM_NAME MATCHES "Darwin" OR
   CMAKE_SYSTEM_NAME MATCHES "BSD")
  file (GLOB Sources_Unix "system/unix/*.c")
  list(APPEND Sources ${Sources_Unix})
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
  file (GLOB Sources_Emscripten "system/emscripten/*.c")
  list(APPEND Sources ${Sources_Emscripten})
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  file (GLOB Sources_Windows "system/windows/*.c")
  list(APPEND Sources ${Sources_Windows})
endif()

set(ZenohPicoSources ${Sources} PARENT_SCOPE)
