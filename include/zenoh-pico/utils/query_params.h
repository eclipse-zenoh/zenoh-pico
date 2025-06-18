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

#ifndef ZENOH_PICO_UTILS_QUERY_PARAMS_H
#define ZENOH_PICO_UTILS_QUERY_PARAMS_H

#include "zenoh-pico/utils/string.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const char *_Z_QUERY_PARAMS_KEY_TIME;

typedef struct {
    _z_str_se_t key;
    _z_str_se_t value;
} _z_query_param_t;

/**
 * Extracts the next query parameter from a `_z_str_se_t` string.
 *
 * Returns a `_z_query_param_t` with positions of the next key/value in the string if present.
 * After invocation `str` will point to the remainder of the string.
 */
_z_query_param_t _z_query_params_next(_z_str_se_t *str);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_UTILS_QUERY_PARAMS_H */
