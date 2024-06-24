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

#include "zenoh-pico/net/reply.h"

#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

_z_reply_t _z_reply_null(void) {
    _z_reply_t r = {._tag = Z_REPLY_TAG_DATA,
                    .data = {
                        .replier_id = {.id = {0}},
                        .has_sample_as_rc = false,
                        .sample.base = _z_sample_null(),
                    }};
    return r;
}

#if Z_FEATURE_QUERY == 1
void _z_reply_data_clear(_z_reply_data_t *reply_data) {
    if (reply_data->has_sample_as_rc) {
        _z_sample_rc_drop(&reply_data->sample.rc);
    } else {
        _z_sample_clear(&reply_data->sample.base);
    }
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

void _z_reply_data_copy(_z_reply_data_t *dst, _z_reply_data_t *src) {
    dst->replier_id = src->replier_id;
    dst->has_sample_as_rc = src->has_sample_as_rc;

    if (src->has_sample_as_rc) {
        _z_sample_rc_copy(&dst->sample.rc, &src->sample.rc);
    } else {
        _z_sample_copy(&dst->sample.base, &src->sample.base);
    }
}

_z_reply_t _z_reply_move(_z_reply_t *src_reply) {
    _z_reply_t reply = *src_reply;
    *src_reply = _z_reply_null();
    return reply;
}

void _z_reply_clear(_z_reply_t *reply) { _z_reply_data_clear(&reply->data); }

void _z_reply_free(_z_reply_t **reply) {
    _z_reply_t *ptr = *reply;

    if (*reply != NULL) {
        _z_reply_clear(ptr);

        z_free(ptr);
        *reply = NULL;
    }
}

void _z_reply_copy(_z_reply_t *dst, _z_reply_t *src) {
    _z_reply_data_copy(&dst->data, &src->data);
    dst->_tag = src->_tag;
}

_Bool _z_pending_reply_eq(const _z_pending_reply_t *one, const _z_pending_reply_t *two) {
    return one->_tstamp.time == two->_tstamp.time;
}

void _z_pending_reply_clear(_z_pending_reply_t *pr) {
    // Free reply
    _z_reply_clear(&pr->_reply);

    // Free the timestamp
    _z_timestamp_clear(&pr->_tstamp);
}

_z_reply_t _z_reply_create(_z_keyexpr_t keyexpr, z_reply_tag_t tag, _z_id_t id, const _z_bytes_t payload,
                           const _z_timestamp_t *timestamp, _z_encoding_t encoding, z_sample_kind_t kind,
                           const _z_bytes_t attachment) {
    _z_reply_t reply = _z_reply_null();
    reply._tag = tag;
    if (tag == Z_REPLY_TAG_DATA) {
        reply.data.replier_id = id;
        // Create reply sample
        reply.data.sample.base.keyexpr = keyexpr;    // FIXME: call z_keyexpr_move or copy
        reply.data.sample.base.encoding = encoding;  // FIXME: call z_encoding_move or copy
        _z_bytes_copy(&sample.payload, &payload);
        reply.data.sample.base.kind = kind;
        reply.data.sample.base.timestamp = _z_timestamp_duplicate(timestamp);
        reply.data.sample.base.attachment._slice = _z_slice_steal((_z_slice_t *)&att._slice);
    }
    return reply;
}
#else
_z_reply_t _z_reply_create(_z_keyexpr_t keyexpr, z_reply_tag_t tag, _z_id_t id, const _z_bytes_t payload,
                           const _z_timestamp_t *timestamp, _z_encoding_t encoding, z_sample_kind_t kind,
                           const _z_bytes_t attachment) {
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(tag);
    _ZP_UNUSED(id);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(timestamp);
    _ZP_UNUSED(encoding);
    _ZP_UNUSED(kind);
    _ZP_UNUSED(attachment);
    return _z_reply_null();
}
#endif
