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

#include "zenoh-pico/net/interest.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_INTEREST == 1
void _z_interest_clear(_z_interest_t *intr) {
    // Nothing to clear
    (void)(intr);
}

void _z_interest_free(_z_interest_t **intr) {
    _z_interest_t *ptr = *intr;

    if (ptr != NULL) {
        _z_interest_clear(ptr);

        zp_free(ptr);
        *intr = NULL;
    }
}
#endif
