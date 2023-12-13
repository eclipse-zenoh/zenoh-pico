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
#include "zenoh-pico/collections/bytes.h"

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
_z_value_t _z_value_null(void) { return (_z_value_t){.payload = _z_bytes_empty(), .encoding = z_encoding_default()}; }
_z_value_t _z_value_steal(_z_value_t *value) {
    _z_value_t ret = *value;
    *value = _z_value_null();
    return ret;
}
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
_z_bytes_t z_attachment_get(z_attachment_t this, _z_bytes_t key) {
    struct _z_seeker_t seeker = {.value = {0}, .key = key};
    z_attachment_iterate(this, _z_attachment_get_seeker, (void *)&seeker);
    return seeker.value;
}
