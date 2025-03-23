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

#include "zenoh-pico/net/sample.h"

#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

void _z_sample_move(_z_sample_t *dst, _z_sample_t *src) {
    _z_keyexpr_move(&dst->keyexpr, &src->keyexpr);
    _z_bytes_move(&dst->payload, &src->payload);
    _z_encoding_move(&dst->encoding, &src->encoding);
    _z_timestamp_move(&dst->timestamp, &src->timestamp);
    _z_bytes_move(&dst->attachment, &src->attachment);
    dst->qos = src->qos;
    dst->reliability = src->reliability;
    dst->kind = src->kind;
}

void _z_sample_clear(_z_sample_t *sample) {
    _z_keyexpr_clear(&sample->keyexpr);
    _z_encoding_clear(&sample->encoding);
    _z_bytes_drop(&sample->payload);
    _z_bytes_drop(&sample->attachment);
}

void _z_sample_free(_z_sample_t **sample) {
    _z_sample_t *ptr = *sample;
    if (ptr != NULL) {
        _z_sample_clear(ptr);
        z_free(ptr);
        *sample = NULL;
    }
}

z_result_t _z_sample_copy(_z_sample_t *dst, const _z_sample_t *src) {
    *dst = _z_sample_null();
    _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst->keyexpr, &src->keyexpr));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_copy(&dst->payload, &src->payload), _z_sample_clear(dst));
    _Z_CLEAN_RETURN_IF_ERR(_z_encoding_copy(&dst->encoding, &src->encoding), _z_sample_clear(dst));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_copy(&dst->attachment, &src->attachment), _z_sample_clear(dst));
    dst->kind = src->kind;
    dst->timestamp = _z_timestamp_duplicate(&src->timestamp);
    return _Z_RES_OK;
}

_z_sample_t _z_sample_duplicate(const _z_sample_t *src) {
    _z_sample_t dst;
    _z_sample_copy(&dst, src);
    return dst;
}
