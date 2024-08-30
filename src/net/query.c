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
        .attachment = _z_bytes_null(),
        ._zn = _z_session_weak_null(),
    };
}

void _z_query_clear_inner(_z_query_t *q) {
    _z_keyexpr_clear(&q->_key);
    _z_value_clear(&q->_value);
    _z_bytes_drop(&q->attachment);
    z_free(q->_parameters);
    _z_session_weak_drop(&q->_zn);
}

void _z_query_clear(_z_query_t *q) {
    // Try to upgrade session weak to rc
    _z_session_rc_t sess_rc = _z_session_weak_upgrade(&q->_zn);
    if (!_Z_RC_IS_NULL(&sess_rc)) {
        // Send REPLY_FINAL message
        _z_zenoh_message_t z_msg = _z_n_msg_make_response_final(q->_request_id);
        if (_z_send_n_msg(_Z_RC_IN_VAL(&q->_zn), &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) !=
            _Z_RES_OK) {
            _Z_ERROR("Query send REPLY_FINAL transport failure !");
        }
        _z_msg_clear(&z_msg);
        _z_session_rc_drop(&sess_rc);
    }
    // Clean up memory
    _z_query_clear_inner(q);
}

int8_t _z_query_copy(_z_query_t *dst, const _z_query_t *src) {
    *dst = _z_query_null();
    _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst->_key, &src->_key));
    _Z_CLEAN_RETURN_IF_ERR(_z_value_copy(&dst->_value, &src->_value), _z_query_clear_inner(dst));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_copy(&dst->attachment, &src->attachment), _z_query_clear_inner(dst));
    dst->_parameters = _z_str_clone(src->_parameters);
    if (dst->_parameters == NULL && src->_parameters != NULL) {
        _z_query_clear_inner(dst);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _z_session_weak_copy(&dst->_zn, &src->_zn);
    if (_Z_RC_IS_NULL(&dst->_zn)) {
        _z_query_clear_inner(dst);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    dst->_anyke = src->_anyke;
    dst->_request_id = src->_request_id;
    dst->_zn = src->_zn;
    return _Z_RES_OK;
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
_z_query_t _z_query_create(_z_value_t *value, _z_keyexpr_t *key, const _z_slice_t *parameters, _z_session_rc_t *zsrc,
                           uint32_t request_id, const _z_bytes_t attachment) {
    _z_query_t q = _z_query_null();
    q._request_id = request_id;
    q._zn = _z_session_rc_clone_as_weak(zsrc);
    q._parameters = (char *)z_malloc(parameters->len + 1);
    memcpy(q._parameters, parameters->start, parameters->len);
    q._parameters[parameters->len] = 0;
    q._anyke = (strstr(q._parameters, Z_SELECTOR_QUERY_MATCH) == NULL) ? false : true;
    q._key = _z_keyexpr_steal(key);
    _z_bytes_copy(&q.attachment, &attachment);
    _z_value_move(&q._value, value);
    return q;
}

_z_queryable_t _z_queryable_null(void) { return (_z_queryable_t){._entity_id = 0, ._zn = _z_session_rc_null()}; }

_Bool _z_queryable_check(const _z_queryable_t *queryable) { return !_Z_RC_IS_NULL(&queryable->_zn); }

void _z_queryable_clear(_z_queryable_t *qbl) {
    // Nothing to clear
    *qbl = _z_queryable_null();
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
