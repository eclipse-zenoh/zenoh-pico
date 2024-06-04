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

_z_sample_t _z_sample_null(void) {
    _z_sample_t s = {
        .keyexpr = _z_keyexpr_null(),
        .payload = _z_bytes_empty(),
        .encoding = _z_encoding_null(),
        .timestamp = _z_timestamp_null(),
        .kind = 0,
        .qos = {0},
#if Z_FEATURE_ATTACHMENT == 1
        .attachment = z_attachment_null(),
#endif
    };
    return s;
}

_Bool _z_sample_check(const _z_sample_t *sample) {
    return _z_keyexpr_check(sample->keyexpr) && _z_bytes_check(sample->payload);
}

void _z_sample_move(_z_sample_t *dst, _z_sample_t *src) {
    dst->keyexpr._id = src->keyexpr._id;          // FIXME: call the z_keyexpr_move
    dst->keyexpr._suffix = src->keyexpr._suffix;  // FIXME: call the z_keyexpr_move
    src->keyexpr._suffix = NULL;                  // FIXME: call the z_keyexpr_move

    _z_bytes_move(&dst->payload, &src->payload);
    _z_encoding_move(&dst->encoding, &src->encoding);

    dst->timestamp.time = src->timestamp.time;  // FIXME: call the z_timestamp_move
    dst->timestamp.id = src->timestamp.id;      // FIXME: call the z_timestamp_move
}

void _z_sample_clear(_z_sample_t *sample) {
    _z_keyexpr_clear(&sample->keyexpr);
    _z_bytes_clear(&sample->payload);
    _z_encoding_clear(&sample->encoding);
    _z_timestamp_clear(&sample->timestamp);
#if Z_FEATURE_ATTACHMENT == 1
    _z_attachment_drop(&sample->attachment);
#endif
}

void _z_sample_free(_z_sample_t **sample) {
    _z_sample_t *ptr = *sample;
    if (ptr != NULL) {
        _z_sample_clear(ptr);
        z_free(ptr);
        *sample = NULL;
    }
}

void _z_sample_copy(_z_sample_t *dst, const _z_sample_t *src) {
    dst->keyexpr = _z_keyexpr_duplicate(src->keyexpr);
    dst->payload = _z_bytes_duplicate(&src->payload);
    dst->timestamp = _z_timestamp_duplicate(&src->timestamp);
    _z_encoding_copy(&dst->encoding, &src->encoding);

    dst->kind = src->kind;
#if Z_FEATURE_ATTACHMENT == 1
    dst->attachment = src->attachment;
#endif
}

_z_sample_t _z_sample_duplicate(const _z_sample_t *src) {
    _z_sample_t dst;
    _z_sample_copy(&dst, src);
    return dst;
}

#if Z_FEATURE_SUBSCRIPTION == 1
_z_sample_t _z_sample_create(const _z_keyexpr_t *key, const _z_bytes_t *payload, const _z_timestamp_t timestamp,
                             const _z_encoding_t encoding, const z_sample_kind_t kind, const _z_qos_t qos,
                             const z_attachment_t att) {
    _z_sample_t s = _z_sample_null();
    _z_keyexpr_copy(&s.keyexpr, key);
    _z_bytes_copy(&s.payload, payload);
    _z_encoding_copy(&s.encoding, &encoding);
    s.kind = kind;
    s.timestamp = timestamp;
    s.qos = qos;
#if Z_FEATURE_ATTACHMENT == 1
    s.attachment = att;  // FIXME: call z_attachment_move or copy
#else
    _ZP_UNUSED(att);
#endif
    return s;
}
#else
_z_sample_t _z_sample_create(const _z_keyexpr_t *key, const _z_bytes_t *payload, const _z_timestamp_t timestamp,
                             const _z_encoding_t encoding, const z_sample_kind_t kind, const _z_qos_t qos,
                             const z_attachment_t att) {
    _ZP_UNUSED(key);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(timestamp);
    _ZP_UNUSED(encoding);
    _ZP_UNUSED(kind);
    _ZP_UNUSED(qos);
    _ZP_UNUSED(att);
    return _z_sample_null();
}
#endif
