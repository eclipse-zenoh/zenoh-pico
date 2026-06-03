//
// Copyright (c) 2026 ZettaScale Technology
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
//

#ifndef INCLUDE_ZENOH_PICO_UTILS_SLEEP_H
#define INCLUDE_ZENOH_PICO_UTILS_SLEEP_H

#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/system/platform.h"

#define _Z_SLEEP_BACKOFF_MIN_MS 100
#define _Z_SLEEP_BACKOFF_MAX_MS 1000

static inline void _z_backoff_advance(uint32_t *sleep_ms) {
    if (*sleep_ms < _Z_SLEEP_BACKOFF_MAX_MS) {
        *sleep_ms <<= 1;
        if (*sleep_ms > _Z_SLEEP_BACKOFF_MAX_MS) {
            *sleep_ms = _Z_SLEEP_BACKOFF_MAX_MS;
        }
    }
}

static inline bool _z_backoff_sleep(z_clock_t *start, int32_t timeout_ms, uint32_t *sleep_ms) {
    uint32_t current_sleep_ms = *sleep_ms;

    if (timeout_ms == 0) {
        return false;
    }

    if (timeout_ms > 0) {
        unsigned long elapsed = z_clock_elapsed_ms(start);
        if (elapsed >= (unsigned long)timeout_ms) {
            return false;
        }

        uint32_t remaining_ms = (uint32_t)timeout_ms - (uint32_t)elapsed;
        if (current_sleep_ms > remaining_ms) {
            current_sleep_ms = remaining_ms;
        }
    }

    z_sleep_ms(current_sleep_ms);
    _z_backoff_advance(sleep_ms);
    return true;
}
#endif /* INCLUDE_ZENOH_PICO_UTILS_SLEEP_H */
