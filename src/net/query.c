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

void _z_queryable_clear(_z_queryable_t *qbl) {
    // Nothing to clear
    (void)(qbl);
}

void _z_queryable_free(_z_queryable_t **qbl) {
    _z_queryable_t *ptr = *qbl;

    if (ptr != NULL) {
        _z_queryable_clear(ptr);

        z_free(ptr);
        *qbl = NULL;
    }
}
