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

_z_reply_data_t _z_reply_data_null(void) {
    return (_z_reply_data_t){.replier_id = {.id = {0}}, ._result.sample = _z_sample_null(), ._tag = _Z_REPLY_TAG_NONE};
}

_z_reply_t _z_reply_null(void) {
    _z_reply_t r = {.data = _z_reply_data_null()};
    return r;
}

#if Z_FEATURE_QUERY == 1
void _z_reply_data_clear(_z_reply_data_t *reply_data) {
    if (reply_data->_tag == _Z_REPLY_TAG_DATA) {
        _z_sample_clear(&reply_data->_result.sample);
    } else if (reply_data->_tag == _Z_REPLY_TAG_ERROR) {
        _z_value_clear(&reply_data->_result.error);
    }
    reply_data->_tag = _Z_REPLY_TAG_NONE;
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

int8_t _z_reply_data_copy(_z_reply_data_t *dst, const _z_reply_data_t *src) {
    *dst = _z_reply_data_null();
    if (src->_tag == _Z_REPLY_TAG_DATA) {
        _Z_RETURN_IF_ERR(_z_sample_copy(&dst->_result.sample, &src->_result.sample));
    } else if (src->_tag == _Z_REPLY_TAG_ERROR) {
        _Z_RETURN_IF_ERR(_z_value_copy(&dst->_result.error, &src->_result.error));
    }
    dst->replier_id = src->replier_id;
    dst->_tag = src->_tag;
    return _Z_RES_OK;
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

int8_t _z_reply_copy(_z_reply_t *dst, const _z_reply_t *src) {
    *dst = _z_reply_null();
    _Z_RETURN_IF_ERR(_z_reply_data_copy(&dst->data, &src->data));
    return _Z_RES_OK;
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

_z_reply_t _z_reply_create(_z_keyexpr_t keyexpr, _z_id_t id, const _z_bytes_t payload, const _z_timestamp_t *timestamp,
                           _z_encoding_t *encoding, z_sample_kind_t kind, const _z_bytes_t attachment) {
    _z_reply_t reply = _z_reply_null();
    reply.data._tag = _Z_REPLY_TAG_DATA;
    reply.data.replier_id = id;

    // Create reply sample
    reply.data._result.sample.keyexpr = _z_keyexpr_steal(&keyexpr);
    reply.data._result.sample.kind = kind;
    reply.data._result.sample.timestamp = _z_timestamp_duplicate(timestamp);
    _z_bytes_copy(&reply.data._result.sample.payload, &payload);
    _z_bytes_copy(&reply.data._result.sample.attachment, &attachment);
    _z_encoding_move(&reply.data._result.sample.encoding, encoding);

    return reply;
}

_z_reply_t _z_reply_err_create(const _z_bytes_t payload, _z_encoding_t *encoding) {
    _z_reply_t reply = _z_reply_null();
    reply.data._tag = _Z_REPLY_TAG_ERROR;
    _z_bytes_copy(&reply.data._result.error.payload, &payload);
    _z_encoding_move(&reply.data._result.error.encoding, encoding);
    return reply;
}
#else
_z_reply_t _z_reply_create(_z_keyexpr_t keyexpr, _z_id_t id, const _z_bytes_t payload, const _z_timestamp_t *timestamp,
                           _z_encoding_t *encoding, z_sample_kind_t kind, const _z_bytes_t attachment) {
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(id);
    _ZP_UNUSED(payload);
    _ZP_UNUSED(timestamp);
    _ZP_UNUSED(encoding);
    _ZP_UNUSED(kind);
    _ZP_UNUSED(attachment);
    return _z_reply_null();
}

_z_reply_t _z_reply_err_create(const _z_bytes_t payload, _z_encoding_t *encoding) {
    _ZP_UNUSED(payload);
    _ZP_UNUSED(encoding);
    return _z_reply_null();
}
#endif
