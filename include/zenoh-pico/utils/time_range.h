//
// Copyright (c) 2025 ZettaScale Technology
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

#ifndef ZENOH_PICO_UTILS_TIME_RANGE_H
#define ZENOH_PICO_UTILS_TIME_RANGE_H

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/string.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    enum { _Z_TIME_BOUND_INCLUSIVE, _Z_TIME_BOUND_EXCLUSIVE, _Z_TIME_BOUND_UNBOUNDED } bound;
    double now_offset;
} _z_time_bound_t;

typedef struct {
    _z_time_bound_t start;
    _z_time_bound_t end;
} _z_time_range_t;

/**
 * Parse a time range from a string.
 *
 * Returns true if the string contained a valid time range, false otherwise.
 * If valid range will contain the result.
 */
bool _z_time_range_from_str(const char *str, size_t len, _z_time_range_t *range);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_UTILS_TIME_RANGE_H */
