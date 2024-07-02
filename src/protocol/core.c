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
//

#include "zenoh-pico/protocol/core.h"

#include <stdint.h>
#include <string.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/utils/logging.h"

#define _Z_ID_LEN (16)

uint8_t _z_id_len(_z_id_t id) {
    uint8_t len = _Z_ID_LEN;
    while (len > 0) {
        --len;
        if (id.id[len] != 0) {
            ++len;
            break;
        }
    }
    return len;
}
_Bool _z_id_check(_z_id_t id) {
    _Bool ret = false;
    for (int i = 0; !ret && i < _Z_ID_LEN; i++) {
        ret |= id.id[i] != 0;
    }
    return ret;
}
_z_id_t _z_id_empty(void) {
    return (_z_id_t){.id = {
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                         0,
                     }};
}

_z_source_info_t _z_source_info_null(void) {
    return (_z_source_info_t){._source_sn = 0, ._entity_id = 0, ._id = _z_id_empty()};
}
_z_timestamp_t _z_timestamp_null(void) { return (_z_timestamp_t){.id = _z_id_empty(), .time = 0}; }
_z_value_t _z_value_null(void) { return (_z_value_t){.payload = _z_bytes_null(), .encoding = _z_encoding_null()}; }
_z_value_t _z_value_steal(_z_value_t *value) {
    _z_value_t ret = *value;
    *value = _z_value_null();
    return ret;
}
int8_t _z_value_copy(_z_value_t *dst, const _z_value_t *src) {
    *dst = _z_value_null();
    _Z_RETURN_IF_ERR(_z_encoding_copy(&dst->encoding, &src->encoding));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_copy(&dst->payload, &src->payload), _z_encoding_clear(&dst->encoding));
    return _Z_RES_OK;
}

int8_t _z_hello_copy(_z_hello_t *dst, const _z_hello_t *src) {
    *dst = _z_hello_null();
    _Z_RETURN_IF_ERR(_z_string_svec_copy(&dst->locators, &src->locators) ? _Z_RES_OK : _Z_ERR_SYSTEM_OUT_OF_MEMORY);
    dst->version = src->version;
    dst->whatami = src->whatami;
    memcpy(&dst->zid.id, &src->zid.id, _Z_ID_LEN);
    return _Z_RES_OK;
}

_z_hello_t _z_hello_null(void) {
    return (_z_hello_t){.zid = _z_id_empty(), .version = 0, .whatami = 0x0, .locators = _z_string_svec_make(0)};
}

void _z_value_move(_z_value_t *dst, _z_value_t *src) {
    _z_encoding_move(&dst->encoding, &src->encoding);
    _z_bytes_move(&dst->payload, &src->payload);
}
