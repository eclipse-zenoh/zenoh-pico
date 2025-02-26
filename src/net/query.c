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

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/utils/logging.h"

static void _z_query_clear_inner(_z_query_t *q) {
    _z_keyexpr_clear(&q->_key);
    _z_value_clear(&q->_value);
    _z_bytes_drop(&q->_attachment);
    _z_string_clear(&q->_parameters);
    _z_session_weak_drop(&q->_zn);
}

z_result_t _z_query_send_reply_final(_z_query_t *q) {
    // Try to upgrade session weak to rc
    _z_session_rc_t sess_rc = _z_session_weak_upgrade_if_open(&q->_zn);
    if (_Z_RC_IS_NULL(&sess_rc)) {
        return _Z_ERR_TRANSPORT_TX_FAILED;
    }
    _z_zenoh_message_t z_msg = _z_n_msg_make_response_final(q->_request_id);
    z_result_t ret = _z_send_n_msg(_Z_RC_IN_VAL(&sess_rc), &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
    _z_msg_clear(&z_msg);
    _z_session_rc_drop(&sess_rc);
    return ret;
}

void _z_query_clear(_z_query_t *q) {
    // Send REPLY_FINAL message
    if (!_Z_RC_IS_NULL(&q->_zn) && _z_query_send_reply_final(q) != _Z_RES_OK) {
        _Z_ERROR("Query send REPLY_FINAL transport failure !");
    }
    // Clean up memory
    _z_query_clear_inner(q);
}

void _z_query_free(_z_query_t **query) {
    _z_query_t *ptr = *query;
    if (ptr != NULL) {
        _z_query_clear(ptr);
        z_free(ptr);
        *query = NULL;
    }
}

#if Z_FEATURE_QUERY == 1
void _z_querier_clear(_z_querier_t *querier) {
    _z_keyexpr_clear(&querier->_key);
    _z_session_weak_drop(&querier->_zn);
    _z_encoding_clear(&querier->_encoding);
    *querier = _z_querier_null();
}

void _z_querier_free(_z_querier_t **querier) {
    _z_querier_t *ptr = *querier;

    if (ptr != NULL) {
        _z_querier_clear(ptr);

        z_free(ptr);
        *querier = NULL;
    }
}
#endif

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
