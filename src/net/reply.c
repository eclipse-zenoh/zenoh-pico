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

#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_QUERY == 1
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

void _z_reply_data_copy(_z_reply_data_t *dst, _z_reply_data_t *src) {
    _z_sample_copy(&dst->sample, &src->sample);
    dst->replier_id = src->replier_id;
}

_z_reply_t *_z_reply_alloc_and_move(_z_reply_t *_reply) {
    _z_reply_t *reply = (_z_reply_t *)z_malloc(sizeof(_z_reply_t));
    if (reply != NULL) {
        *reply = *_reply;
        (void)memset(_reply, 0, sizeof(_z_reply_t));
    }
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
#endif
