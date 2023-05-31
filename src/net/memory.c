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

void _z_sample_move(_z_sample_t *dst, _z_sample_t *src) {
    dst->keyexpr._id = src->keyexpr._id;          // FIXME: call the z_keyexpr_move
    dst->keyexpr._suffix = src->keyexpr._suffix;  // FIXME: call the z_keyexpr_move
    src->keyexpr._suffix = NULL;                  // FIXME: call the z_keyexpr_move

    _z_bytes_move(&dst->payload, &src->payload);

    dst->encoding.prefix = src->encoding.prefix;                  // FIXME: call the z_encoding_move
    _z_bytes_move(&dst->encoding.suffix, &src->encoding.suffix);  // FIXME: call the z_encoding_move

    dst->timestamp.time = src->timestamp.time;  // FIXME: call the z_timestamp_move
    dst->timestamp.id = src->timestamp.id;      // FIXME: call the z_timestamp_move
}

void _z_sample_clear(_z_sample_t *sample) {
    _z_keyexpr_clear(&sample->keyexpr);
    _z_bytes_clear(&sample->payload);
    _z_bytes_clear(&sample->encoding.suffix);  // FIXME: call the z_encoding_clear
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

void _z_hello_clear(_z_hello_t *hello) {
    if (hello->zid.len > 0) {
        _z_bytes_clear(&hello->zid);
    }
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
    _z_bytes_clear(&reply_data->replier_id);
}

void _z_reply_data_free(_z_reply_data_t **reply_data) {
    _z_reply_data_t *ptr = *reply_data;

    if (ptr != NULL) {
        _z_reply_data_clear(ptr);

        z_free(ptr);
        *reply_data = NULL;
    }
}
