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

#ifndef ZENOH_PICO_ENCODING_NETAPI_H
#define ZENOH_PICO_ENCODING_NETAPI_H

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/string.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _Z_ENCODING_ID_DEFAULT 0

/**
 * A zenoh encoding.
 */
typedef struct _z_encoding_t {
    _z_string_t schema;
    uint16_t id;
} _z_encoding_t;

// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_encoding_t _z_encoding_null(void) { return (_z_encoding_t){0}; }
static inline bool _z_encoding_check(const _z_encoding_t *encoding) {
    return ((encoding->id != _Z_ENCODING_ID_DEFAULT) || _z_string_check(&encoding->schema));
}
static inline void _z_encoding_clear(_z_encoding_t *encoding) { _z_string_clear(&encoding->schema); }
_z_encoding_t _z_encoding_wrap(uint16_t id, const char *schema);
z_result_t _z_encoding_make(_z_encoding_t *encoding, uint16_t id, const char *schema, size_t len);
z_result_t _z_encoding_copy(_z_encoding_t *dst, const _z_encoding_t *src);
z_result_t _z_encoding_move(_z_encoding_t *dst, _z_encoding_t *src);
static inline _z_encoding_t _z_encoding_alias(const _z_encoding_t *src) {
    _z_encoding_t dst;
    dst.id = src->id;
    if (_z_string_check(&src->schema)) {
        dst.schema = _z_string_alias(src->schema);
    } else {
        dst.schema = _z_string_null();
    }
    return dst;
}
static inline _z_encoding_t _z_encoding_steal(_z_encoding_t *val) {
    _z_encoding_t ret = *val;
    val->schema._slice.len = 0;
    val->schema._slice.start = NULL;
    return ret;
}

// Non-owning view on the encoding.
typedef struct _z_view_encoding_t {
    _z_view_string_t schema;
    uint16_t id;
} _z_view_encoding_t;

static inline _z_view_encoding_t _z_view_encoding_null(void) { return (_z_view_encoding_t){0}; }
static inline bool _z_view_encoding_check(const _z_view_encoding_t *encoding) {
    return ((encoding->id != _Z_ENCODING_ID_DEFAULT) || !_z_view_string_is_empty(&encoding->schema));
}

static inline _z_view_encoding_t _z_view_encoding_from_encoding(const _z_encoding_t *encoding) {
    _z_view_encoding_t view_encoding;
    view_encoding.schema = _z_view_string_from_string(&encoding->schema);
    view_encoding.id = encoding->id;
    return view_encoding;
}

// Builds a non-owning `_z_encoding_t` aliasing the data of a view encoding. This is meant to bridge view encodings with
// the APIs that only accept `_z_encoding_t`; the returned encoding must not outlive the data referenced by the view.
static inline _z_encoding_t _z_encoding_alias_view_encoding(const _z_view_encoding_t *encoding) {
    _z_encoding_t en;
    en.id = encoding->id;
    en.schema = _z_string_alias_view_string(&encoding->schema);
    return en;
}

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_ENCODING_NETAPI_H */
