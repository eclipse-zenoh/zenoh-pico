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

_z_query_t _z_query_null(void) {
    return (_z_query_t){
        ._anyke = false,
        ._key = _z_keyexpr_null(),
        ._parameters = NULL,
        ._request_id = 0,
        ._value = _z_value_null(),
        ._zn = NULL,
    };
}

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
    _z_bytes_drop(&q->attachment);
}

void _z_query_copy(_z_query_t *dst, const _z_query_t *src) {
    dst->_anyke = src->_anyke;
    dst->_key = _z_keyexpr_duplicate(src->_key);
    dst->_parameters = src->_parameters;
    dst->_request_id = src->_request_id;
    dst->_zn = src->_zn;
    _z_value_copy(&dst->_value, &src->_value);
}

void _z_query_free(_z_query_t **query) {
    _z_query_t *ptr = *query;
    if (ptr != NULL) {
        _z_query_clear(ptr);
        z_free(ptr);
        *query = NULL;
    }
}

#if Z_FEATURE_QUERYABLE == 1
_z_query_t _z_query_create(const _z_value_t *value, _z_keyexpr_t *key, const _z_slice_t *parameters, _z_session_t *zn,
                           uint32_t request_id, const _z_bytes_t attachment) {
    _z_query_t q = _z_query_null();
    q._request_id = request_id;
    q._zn = zn;
    q._parameters = (char *)z_malloc(parameters->len + 1);
    memcpy(q._parameters, parameters->start, parameters->len);  // TODO: Might be movable, Issue #482
    q._parameters[parameters->len] = 0;
    q._anyke = (strstr(q._parameters, Z_SELECTOR_QUERY_MATCH) == NULL) ? false : true;
    q._key = _z_keyexpr_steal(key);
    _z_bytes_copy(&q.attachment, &attachment);
    _z_value_copy(&q._value, value);  // FIXME: Move encoding, Issue #482
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
