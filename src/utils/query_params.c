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

#include "zenoh-pico/utils/query_params.h"

#include "zenoh-pico/utils/pointers.h"

const char *_Z_QUERY_PARAMS_KEY_TIME = "_time";

static const char *_Z_QUERY_PARAMS_LIST_SEPARATOR = ";";
static const char *_Z_QUERY_PARAMS_FIELD_SEPARATOR = "=";

_z_query_param_t _z_query_params_next(_z_str_se_t *str) {
    _z_query_param_t result = {0};

    _z_splitstr_t params = {.s = *str, .delimiter = _Z_QUERY_PARAMS_LIST_SEPARATOR};
    _z_str_se_t param = _z_splitstr_next(&params);
    str->start = params.s.start;
    str->end = params.s.end;

    if (param.start != NULL) {
        _z_splitstr_t kvpair = {.s = param, .delimiter = _Z_QUERY_PARAMS_FIELD_SEPARATOR};
        _z_str_se_t key = _z_splitstr_next(&kvpair);

        // Set key if length > 0
        if (_z_ptr_char_diff(key.end, key.start) > 0) {
            result.key.start = key.start;
            result.key.end = key.end;

            // Set value if length > 0
            if (_z_ptr_char_diff(kvpair.s.end, kvpair.s.start) > 0) {
                result.value.start = kvpair.s.start;
                result.value.end = kvpair.s.end;
            }
        }
    }
    return result;
}
