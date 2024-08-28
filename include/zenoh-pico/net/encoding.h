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

#define _Z_ENCODING_ID_DEFAULT 0

/**
 * A zenoh encoding.
 */
typedef struct _z_encoding_t {
    _z_string_t schema;
    uint16_t id;
} _z_encoding_t;

int8_t _z_encoding_make(_z_encoding_t *encoding, uint16_t id, const char *schema, size_t len);
_z_encoding_t _z_encoding_wrap(uint16_t id, const char *schema);
_z_encoding_t _z_encoding_null(void);
void _z_encoding_clear(_z_encoding_t *encoding);
_Bool _z_encoding_check(const _z_encoding_t *encoding);
int8_t _z_encoding_copy(_z_encoding_t *dst, const _z_encoding_t *src);
void _z_encoding_move(_z_encoding_t *dst, _z_encoding_t *src);
_z_encoding_t _z_encoding_steal(_z_encoding_t *val);

#endif /* ZENOH_PICO_ENCODING_NETAPI_H */
