//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/net/encoding.h"

#include <string.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

int8_t _z_encoding_make(_z_encoding_t *encoding, uint16_t id, const char *schema, size_t len) {
    encoding->id = id;
    // Clone schema
    if (schema != NULL) {
        encoding->schema = _z_string_n_make(schema, len);
        if (encoding->schema.len != len) {
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
    } else {
        encoding->schema = _z_string_null();
    }
    return _Z_RES_OK;
}

_z_encoding_t _z_encoding_wrap(uint16_t id, const char *schema) {
    return (_z_encoding_t){.id = id, .schema = (schema == NULL) ? _z_string_null() : _z_string_wrap((char *)schema)};
}

_z_encoding_t _z_encoding_null(void) { return _z_encoding_wrap(_Z_ENCODING_ID_DEFAULT, NULL); }

void _z_encoding_clear(_z_encoding_t *encoding) { _z_string_clear(&encoding->schema); }

_Bool _z_encoding_check(const _z_encoding_t *encoding) {
    return ((encoding->id != _Z_ENCODING_ID_DEFAULT) || _z_string_check(&encoding->schema));
}

int8_t _z_encoding_copy(_z_encoding_t *dst, const _z_encoding_t *src) {
    *dst = _z_encoding_null();
    _Z_RETURN_IF_ERR(_z_string_copy(&dst->schema, &src->schema));
    dst->id = src->id;
    return _Z_RES_OK;
}

void _z_encoding_move(_z_encoding_t *dst, _z_encoding_t *src) {
    dst->id = src->id;
    src->id = _Z_ENCODING_ID_DEFAULT;
    _z_string_move(&dst->schema, &src->schema);
}

_z_encoding_t _z_encoding_steal(_z_encoding_t *val) {
    _z_encoding_t ret = {
        .id = val->id,
        .schema = _z_string_steal(&val->schema),
    };
    val->id = _Z_ENCODING_ID_DEFAULT;
    return ret;
}
