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

#include "zenoh-pico/net/memory.h"

#include <stddef.h>

#include "zenoh-pico/protocol/core.h"

void _z_sample_move(_z_sample_t *dst, _z_sample_t *src) {
    dst->keyexpr._id = src->keyexpr._id;          // FIXME: call the z_keyexpr_move
    dst->keyexpr._suffix = src->keyexpr._suffix;  // FIXME: call the z_keyexpr_move
    src->keyexpr._suffix = NULL;                  // FIXME: call the z_keyexpr_move

    _z_bytes_move(&dst->payload, &src->payload);

    dst->encoding.id = src->encoding.id;                          // FIXME: call the z_encoding_move
    _z_bytes_move(&dst->encoding.schema, &src->encoding.schema);  // FIXME: call the z_encoding_move

    dst->timestamp.time = src->timestamp.time;  // FIXME: call the z_timestamp_move
    dst->timestamp.id = src->timestamp.id;      // FIXME: call the z_timestamp_move
}

void _z_sample_clear(_z_sample_t *sample) {
    _z_keyexpr_clear(&sample->keyexpr);
    _z_bytes_clear(&sample->payload);
    _z_bytes_clear(&sample->encoding.schema);  // FIXME: call the z_encoding_clear
    _z_timestamp_clear(&sample->timestamp);
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

    // TODO(sashacmc): should be changed after encoding rework
    dst->encoding.id = src->encoding.id;
    _z_bytes_copy(&dst->encoding.schema, &src->encoding.schema);

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

void _z_hello_clear(_z_hello_t *hello) {
    if (hello->locators.len > 0) {
        _z_str_array_clear(&hello->locators);
    }
}

void _z_hello_free(_z_hello_t **hello) {
    _z_hello_t *ptr = *hello;

    if (ptr != NULL) {
        _z_hello_clear(ptr);

        z_free(ptr);
        *hello = NULL;
    }
}

void _z_reply_data_clear(_z_reply_data_t *reply_data) {
    _z_sample_clear(&reply_data->sample);
    reply_data->replier_id = _z_id_empty();
}

void _z_reply_data_free(_z_reply_data_t **reply_data) {
    _z_reply_data_t *ptr = *reply_data;

    if (ptr != NULL) {
        _z_reply_data_clear(ptr);

        z_free(ptr);
        *reply_data = NULL;
    }
}

void _z_value_clear(_z_value_t *value) {
    _z_bytes_clear(&value->encoding.schema);
    _z_bytes_clear(&value->payload);
}

void _z_value_free(_z_value_t **value) {
    _z_value_t *ptr = *value;

    if (ptr != NULL) {
        _z_value_clear(ptr);

        z_free(ptr);
        *value = NULL;
    }
}
