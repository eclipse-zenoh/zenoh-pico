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

#include "zenoh-pico/protocol/codec/core.h"

#include <stdint.h>
#include <string.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/utils/endianness.h"
#include "zenoh-pico/utils/logging.h"

#define _Z_ID_LEN (16)

const _z_id_t empty_id = {0};

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

uint64_t _z_timestamp_ntp64_from_time(uint32_t seconds, uint32_t nanos) {
    const uint64_t FRAC_PER_SEC = (uint64_t)1 << 32;
    const uint64_t NANOS_PER_SEC = 1000000000;

    uint32_t fractions = (uint32_t)((uint64_t)nanos * FRAC_PER_SEC / NANOS_PER_SEC + 1);
    return ((uint64_t)seconds << 32) | fractions;
}

_z_value_t _z_value_steal(_z_value_t *value) {
    _z_value_t ret = *value;
    *value = _z_value_null();
    return ret;
}
z_result_t _z_value_copy(_z_value_t *dst, const _z_value_t *src) {
    *dst = _z_value_null();
    _Z_RETURN_IF_ERR(_z_encoding_copy(&dst->encoding, &src->encoding));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_copy(&dst->payload, &src->payload), _z_encoding_clear(&dst->encoding));
    return _Z_RES_OK;
}
_z_value_t _z_value_alias(_z_value_t *src) {
    _z_value_t dst;
    dst.payload = _z_bytes_alias(&src->payload);
    dst.encoding = _z_encoding_alias(&src->encoding);
    return dst;
}

z_result_t _z_hello_copy(_z_hello_t *dst, const _z_hello_t *src) {
    *dst = _z_hello_null();
    _Z_RETURN_IF_ERR(_z_string_svec_copy(&dst->_locators, &src->_locators, true));
    dst->_version = src->_version;
    dst->_whatami = src->_whatami;
    memcpy(&dst->_zid.id, &src->_zid.id, _Z_ID_LEN);
    return _Z_RES_OK;
}

z_result_t _z_hello_move(_z_hello_t *dst, _z_hello_t *src) {
    *dst = *src;
    *src = _z_hello_null();
    return _Z_RES_OK;
}

z_result_t _z_value_move(_z_value_t *dst, _z_value_t *src) {
    *dst = _z_value_null();
    _Z_RETURN_IF_ERR(_z_bytes_move(&dst->payload, &src->payload));
    _Z_CLEAN_RETURN_IF_ERR(_z_encoding_move(&dst->encoding, &src->encoding), _z_value_clear(dst));
    return _Z_RES_OK;
}

z_result_t _z_source_info_copy(_z_source_info_t *dst, const _z_source_info_t *src) {
    dst->_source_id = src->_source_id;
    dst->_source_sn = src->_source_sn;
    return _Z_RES_OK;
}

z_result_t _z_source_info_move(_z_source_info_t *dst, _z_source_info_t *src) {
    dst->_source_id = src->_source_id;
    dst->_source_sn = src->_source_sn;
    return _Z_RES_OK;
}
