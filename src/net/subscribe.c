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

#include "zenoh-pico/net/subscribe.h"

_z_subinfo_t _z_subinfo_push_default(void) {
    _z_subinfo_t si;
    si.reliability = Z_RELIABILITY_RELIABLE;
    si.mode = Z_SUBMODE_PUSH;
    si.period = (_z_period_t){.origin = 0, .period = 0, .duration = 0};
    return si;
}

_z_subinfo_t _z_subinfo_pull_default(void) {
    _z_subinfo_t si;
    si.reliability = Z_RELIABILITY_RELIABLE;
    si.mode = Z_SUBMODE_PULL;
    si.period = (_z_period_t){.origin = 0, .period = 0, .duration = 0};
    return si;
}

void _z_subscriber_clear(_z_subscriber_t *sub) {
    // Nothing to clear
    (void)(sub);
}

void _z_subscriber_free(_z_subscriber_t **sub) {
    _z_subscriber_t *ptr = *sub;

    if (ptr != NULL) {
        _z_subscriber_clear(ptr);

        z_free(ptr);
        *sub = NULL;
    }
}
