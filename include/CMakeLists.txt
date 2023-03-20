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

file(GLOB PublicHeaders "*.h"
  "zenoh-pico/*.h"
  "zenoh-pico/api/*.h"
  "zenoh-pico/collections/*.h"
  "zenoh-pico/link/*.h"
  "zenoh-pico/link/config/*.h"
  "zenoh-pico/protocol/*.h"
  "zenoh-pico/session/*.h"
  "zenoh-pico/system/*.h"
  "zenoh-pico/system/link/*.h"
  "zenoh-pico/transport/*.h"
  "zenoh-pico/utils/*.h"
)

set(PublicHeaders ${PublicHeaders} PARENT_SCOPE)
