//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>

#include "zenoh-pico/session/query.h"

#include "zenoh-pico/api/primitives.h"

_z_target_t _z_target_default(void) {
    return (_z_target_t){._kind = Z_QUERYABLE_ALL_KINDS, ._target = z_query_target_default()};
}
