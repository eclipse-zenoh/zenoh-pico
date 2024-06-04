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
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/utils/logging.h"

uint8_t _z_id_len(_z_id_t id) {
    uint8_t len = 16;
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
    for (int i = 0; !ret && i < 16; i++) {
        ret |= id.id[i] != 0;
    }
    return ret;
}
_z_id_t _z_id_empty() {
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
_z_source_info_t _z_source_info_null() {
    return (_z_source_info_t){._source_sn = 0, ._entity_id = 0, ._id = _z_id_empty()};
}
_z_timestamp_t _z_timestamp_null() { return (_z_timestamp_t){.id = _z_id_empty(), .time = 0}; }
_z_value_t _z_value_null(void) { return (_z_value_t){.payload = _z_bytes_empty(), .encoding = _z_encoding_null()}; }
_z_value_t _z_value_steal(_z_value_t *value) {
    _z_value_t ret = *value;
    *value = _z_value_null();
    return ret;
}
void _z_value_copy(_z_value_t *dst, const _z_value_t *src) {
    _z_encoding_copy(&dst->encoding, &src->encoding);
    _z_bytes_copy(&dst->payload, &src->payload);
}

z_attachment_t z_attachment_null(void) { return (z_attachment_t){.data = NULL, .iteration_driver = NULL}; }

#if Z_FEATURE_ATTACHMENT == 1
struct _z_seeker_t {
    _z_bytes_t key;
    _z_bytes_t value;
};
int8_t _z_attachment_get_seeker(_z_bytes_t key, _z_bytes_t value, void *ctx) {
    struct _z_seeker_t *seeker = (struct _z_seeker_t *)ctx;
    _z_bytes_t seeked = seeker->key;
    if (key.len == seeked.len && memcmp(key.start, seeked.start, seeked.len)) {
        seeker->value = (_z_bytes_t){.start = value.start, .len = value.len, ._is_alloc = false};
        return 1;
    }
    return 0;
}
_z_bytes_t z_attachment_get(z_attachment_t this_, _z_bytes_t key) {
    struct _z_seeker_t seeker = {.value = {0}, .key = key};
    z_attachment_iterate(this_, _z_attachment_get_seeker, (void *)&seeker);
    return seeker.value;
}
int8_t _z_attachment_estimate_length_body(_z_bytes_t key, _z_bytes_t value, void *ctx) {
    size_t *len = (size_t *)ctx;
    *len += _z_zint_len(key.len) + key.len + _z_zint_len(value.len) + value.len;
    return 0;
}
size_t _z_attachment_estimate_length(z_attachment_t att) {
    size_t len = 0;
    z_attachment_iterate(att, _z_attachment_estimate_length_body, &len);
    return len;
}

int8_t _z_encoded_attachment_iteration_driver(const void *this_, z_attachment_iter_body_t body, void *ctx) {
    _z_zbuf_t data = _z_zbytes_as_zbuf(*(_z_bytes_t *)this_);
    while (_z_zbuf_can_read(&data)) {
        _z_bytes_t key = _z_bytes_empty();
        _z_bytes_t value = _z_bytes_empty();
        _z_bytes_decode(&key, &data);
        _z_bytes_decode(&value, &data);
        int8_t ret = body(key, value, ctx);
        if (ret != 0) {
            return ret;
        }
    }
    return 0;
}

z_attachment_t _z_encoded_as_attachment(const _z_owned_encoded_attachment_t *att) {
    if (att->is_encoded) {
        // Recopy z_bytes data in allocated memory to avoid it going out of scope
        z_bytes_t *att_data = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
        if (att_data == NULL) {
            return att->body.decoded;
        }
        *att_data = att->body.encoded;
        return (z_attachment_t){.data = att_data, .iteration_driver = _z_encoded_attachment_iteration_driver};
    } else {
        return att->body.decoded;
    }
}

_Bool z_attachment_check(const z_attachment_t *attachment) { return attachment->iteration_driver != NULL; }

int8_t z_attachment_iterate(z_attachment_t this_, z_attachment_iter_body_t body, void *ctx) {
    return this_.iteration_driver(this_.data, body, ctx);
}

void _z_attachment_drop(z_attachment_t *att) {
    if (att->iteration_driver == _z_encoded_attachment_iteration_driver) {
        _z_bytes_clear((z_bytes_t *)att->data);
        z_free((z_bytes_t *)att->data);
    }
}

#else  // Z_FEATURE_ATTACHMENT == 1
z_attachment_t _z_encoded_as_attachment(const _z_owned_encoded_attachment_t *att) {
    _ZP_UNUSED(att);
    return z_attachment_null();
}
#endif
