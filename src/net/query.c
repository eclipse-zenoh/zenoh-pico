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
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/utils/logging.h"

static void _z_query_clear_inner(_z_query_t *q) {
    _z_keyexpr_clear(&q->_key);
    _z_value_clear(&q->_value);
    _z_bytes_drop(&q->_attachment);
    _z_string_clear(&q->_parameters);
    _z_session_rc_drop(&q->_zn);
}

z_result_t _z_query_send_reply_final(_z_query_t *q) {
    if (_Z_RC_IS_NULL(&q->_zn)) {
        return _Z_ERR_TRANSPORT_TX_FAILED;
    }
    _z_zenoh_message_t z_msg = _z_n_msg_make_response_final(q->_request_id);
    z_result_t ret = _z_send_n_msg(_Z_RC_IN_VAL(&q->_zn), &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
    _z_msg_clear(&z_msg);
    return ret;
}

void _z_query_clear(_z_query_t *q) {
    // Send REPLY_FINAL message
    if (_z_query_send_reply_final(q) != _Z_RES_OK) {
        _Z_ERROR("Query send REPLY_FINAL transport failure !");
    }
    // Clean up memory
    _z_query_clear_inner(q);
}

z_result_t _z_query_copy(_z_query_t *dst, const _z_query_t *src) {
    *dst = _z_query_null();
    _Z_RETURN_IF_ERR(_z_keyexpr_copy(&dst->_key, &src->_key));
    _Z_CLEAN_RETURN_IF_ERR(_z_value_copy(&dst->_value, &src->_value), _z_query_clear_inner(dst));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_copy(&dst->_attachment, &src->_attachment), _z_query_clear_inner(dst));
    _Z_CLEAN_RETURN_IF_ERR(_z_string_copy(&dst->_parameters, &src->_parameters), _z_query_clear_inner(dst));
    _z_session_rc_copy(&dst->_zn, &src->_zn);
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
void _z_queryable_clear(_z_queryable_t *qbl) {
    _z_session_weak_drop(&qbl->_zn);
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
