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

z_result_t _z_reply_data_copy(_z_reply_data_t *dst, const _z_reply_data_t *src) {
    if (src->_tag == _Z_REPLY_TAG_DATA) {
        _Z_RETURN_IF_ERR(_z_sample_copy(&dst->_result.sample, &src->_result.sample));
    } else if (src->_tag == _Z_REPLY_TAG_ERROR) {
        _Z_RETURN_IF_ERR(_z_value_copy(&dst->_result.error, &src->_result.error));
    }
    dst->_tag = src->_tag;
    dst->replier_id = src->replier_id;
    return _Z_RES_OK;
}

void _z_reply_move(_z_reply_t *dst, _z_reply_t *src) {
    *dst = *src;
    *src = _z_reply_null();
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

z_result_t _z_reply_copy(_z_reply_t *dst, const _z_reply_t *src) { return _z_reply_data_copy(&dst->data, &src->data); }

bool _z_pending_reply_eq(const _z_pending_reply_t *one, const _z_pending_reply_t *two) {
    return one->_tstamp.time == two->_tstamp.time;
}

void _z_pending_reply_clear(_z_pending_reply_t *pr) {
    // Free reply
    _z_reply_clear(&pr->_reply);

    // Free the timestamp
    _z_timestamp_clear(&pr->_tstamp);
}

#endif  // Z_FEATURE_QUERY == 1
