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

int8_t _z_encoding_make(_z_encoding_t *encoding, z_encoding_id_t id, const char *schema) {
    encoding->id = id;
    // Clone schema
    if (schema != NULL) {
        encoding->schema = _z_bytes_make(strlen(schema) + 1);
        if (encoding->schema.start == NULL) {
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
        strcpy((char *)encoding->schema.start, schema);
    } else {
        encoding->schema = _z_bytes_empty();
    }
    return _Z_RES_OK;
}

_z_encoding_t _z_encoding_wrap(z_encoding_id_t id, const char *schema) {
    return (_z_encoding_t){
        .id = id, .schema = _z_bytes_wrap((const uint8_t *)schema, (schema == NULL) ? (size_t)0 : strlen(schema))};
}

_z_encoding_t _z_encoding_null(void) { return _z_encoding_wrap(Z_ENCODING_ID_DEFAULT, NULL); }

void _z_encoding_clear(_z_encoding_t *encoding) { _z_bytes_clear(&encoding->schema); };

_Bool _z_encoding_check(const _z_encoding_t *encoding) {
    return ((encoding->id != Z_ENCODING_ID_DEFAULT) || _z_bytes_check(encoding->schema));
}

void _z_encoding_copy(_z_encoding_t *dst, const _z_encoding_t *src) {
    dst->id = src->id;
    _z_bytes_copy(&dst->schema, &src->schema);
}

void _z_encoding_move(_z_encoding_t *dst, _z_encoding_t *src) {
    dst->id = src->id;
    src->id = Z_ENCODING_ID_DEFAULT;
    _z_bytes_move(&dst->schema, &src->schema);
}
