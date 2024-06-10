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

#include "zenoh-pico/net/query.h"

#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

void _z_query_clear(_z_query_t *q) {
    // Send REPLY_FINAL message
    _z_zenoh_message_t z_msg = _z_n_msg_make_response_final(q->_request_id);
    if (_z_send_n_msg(q->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
        _Z_ERROR("Query send REPLY_FINAL transport failure !");
    }
    // Clean up memory
    _z_msg_clear(&z_msg);
    z_free(q->_parameters);
    _z_keyexpr_clear(&q->_key);
    _z_value_clear(&q->_value);
#if Z_FEATURE_ATTACHMENT == 1
    _z_attachment_drop(&q->attachment);
#endif
}

#if Z_FEATURE_QUERYABLE == 1
_z_query_t _z_query_create(const _z_value_t *value, const _z_keyexpr_t *key, const _z_slice_t *parameters,
                           _z_session_t *zn, uint32_t request_id, z_attachment_t att) {
    _z_query_t q;
    q._request_id = request_id;
    q._zn = zn;  // Ideally would have been an rc
    q._parameters = (char *)z_malloc(parameters->len + 1);
    memcpy(q._parameters, parameters->start, parameters->len);
    q._parameters[parameters->len] = 0;
    q._anyke = (strstr(q._parameters, Z_SELECTOR_QUERY_MATCH) == NULL) ? false : true;
#if Z_FEATURE_ATTACHMENT == 1
    q.attachment = att;
#else
    _ZP_UNUSED(att);
#endif
    _z_keyexpr_copy(&q._key, key);
    _z_value_copy(&q._value, value);
    return q;
}

void _z_queryable_clear(_z_queryable_t *qbl) {
    // Nothing to clear
    (void)(qbl);
}

void _z_queryable_free(_z_queryable_t **qbl) {
    _z_queryable_t *ptr = *qbl;

    if (ptr != NULL) {
        _z_queryable_clear(ptr);

        z_free(ptr);
        *qbl = NULL;
    }
}
#endif
