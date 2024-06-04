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
#include "zenoh-pico/collections/bytes.h"

/**
 * A zenoh encoding.
 */
typedef struct _z_encoding_t {
    _z_bytes_t schema;
    uint16_t id;
} _z_encoding_t;

int8_t _z_encoding_make(_z_encoding_t *encoding, z_encoding_id_t id, const char *schema);
_z_encoding_t _z_encoding_wrap(z_encoding_id_t id, const char *schema);
_z_encoding_t _z_encoding_null(void);
void _z_encoding_clear(_z_encoding_t *encoding);
_Bool _z_encoding_check(const _z_encoding_t *encoding);
void _z_encoding_copy(_z_encoding_t *dst, const _z_encoding_t *src);
void _z_encoding_move(_z_encoding_t *dst, _z_encoding_t *src);

#endif /* ZENOH_PICO_ENCODING_NETAPI_H */
