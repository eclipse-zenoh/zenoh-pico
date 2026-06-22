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
#include "zenoh-pico/session/loopback.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/utils/locality.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_QUERYABLE == 1

z_result_t _z_received_query_count_increase(_z_session_t *zn, const _z_query_owned_t *query) {
    _Z_RETURN_IF_ERR(_z_session_mutex_lock_if_open(zn));
    z_result_t ret = _Z_RES_OK;
    uint32_t *count = _z_rid_to_count_hmap_get(&zn->_received_queries_id_to_count, &query->_id);
    if (count != NULL) {
        (*count)++;
    } else {
        ret = _Z_ERR_ENTITY_UNKNOWN;
    }
    _z_session_mutex_unlock(zn);
    return ret;
}

z_result_t _z_session_send_reply_final(_z_session_t *session, const _z_query_id_t *query_id) {
    if (query_id->peer_id == 0) {
        return _z_session_deliver_reply_final_locally(session, query_id->rid);
    } else {
        _z_zenoh_message_t z_msg;
        _z_n_msg_make_response_final(&z_msg, query_id->rid);
        z_result_t ret = _z_send_n_msg(session, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK,
                                       (void *)(query_id->peer_id));
        return ret;
    }
}

z_result_t _z_received_query_count_decrease(_z_session_t *zn, const _z_query_owned_t *query) {
    _Z_RETURN_IF_ERR(_z_session_mutex_lock_if_open(zn));
    z_result_t ret = _Z_RES_OK;
    bool send_response_final = false;
    _z_rid_to_count_hmap_iter_t it = _z_rid_to_count_hmap_get_iter(&zn->_received_queries_id_to_count, &query->_id);
    if (it != _z_rid_to_count_hmap_end(&zn->_received_queries_id_to_count)) {
        if (_z_rid_to_count_hmap_at(&zn->_received_queries_id_to_count, it)->val == 1) {
            _z_rid_to_count_hmap_remove_at(&zn->_received_queries_id_to_count, it, NULL, NULL);
            send_response_final = true;
        } else {
            _z_rid_to_count_hmap_at(&zn->_received_queries_id_to_count, it)->val--;
        }
    } else {
        ret = _Z_ERR_ENTITY_UNKNOWN;
    }
    _z_session_mutex_unlock(zn);
    if (send_response_final) {
        return _z_session_send_reply_final(zn, &query->_id);
    }
    return ret;
}

void _z_query_owned_clear(_z_query_owned_t *q) {
    _z_session_rc_t zn = _z_session_weak_upgrade_if_open(&q->_zn);
    if (!_Z_RC_IS_NULL(&zn)) {
        _z_received_query_count_decrease(_Z_RC_IN_VAL(&zn), q);
        _z_session_rc_drop(&zn);
    }
    // Clean up memory
    _z_declared_keyexpr_clear(&q->_key);
    _z_value_clear(&q->_value);
    _z_bytes_clear(&q->_attachment);
    _z_string_clear(&q->_parameters);
    _z_session_weak_drop(&q->_zn);
}

void _z_query_owned_move(_z_query_owned_t *dst, _z_query_owned_t *src) {
    *dst = *src;
    *src = _z_query_owned_null();
}

z_result_t _z_query_owned_copy(_z_query_owned_t *dst, const _z_query_owned_t *src) {
    *dst = _z_query_owned_null();
    _z_session_rc_t zn = _z_session_weak_upgrade_if_open(&src->_zn);
    if (!_Z_RC_IS_NULL(&zn)) {
        _Z_CLEAN_RETURN_IF_ERR(_z_received_query_count_increase(_Z_RC_IN_VAL(&zn), src), _z_session_rc_drop(&zn));
        _z_session_rc_drop(&zn);
    } else {
        _Z_ERROR_RETURN(_Z_ERR_ENTITY_UNKNOWN);
    }
    dst->_id = src->_id;
    dst->_zn = _z_session_weak_clone(&src->_zn);

    _Z_CLEAN_RETURN_IF_ERR(_z_declared_keyexpr_copy(&dst->_key, &src->_key), _z_query_owned_clear(dst));
    _Z_CLEAN_RETURN_IF_ERR(_z_value_copy(&dst->_value, &src->_value), _z_query_owned_clear(dst));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_copy(&dst->_attachment, &src->_attachment), _z_query_owned_clear(dst));
    _Z_CLEAN_RETURN_IF_ERR(_z_string_copy(&dst->_parameters, &src->_parameters), _z_query_owned_clear(dst));
    dst->_source_info = src->_source_info;
    dst->_qos = src->_qos;
    dst->_anyke = src->_anyke;
    return _Z_RES_OK;
}

z_result_t _z_query_move_or_copy(_z_query_t *dst, _z_query_t *src) {
    _ZP_VARIANT_VISIT(_z_query, src,
        (owned, *dst = _z_query_from_owned(_)),
        (view, {
            _z_query_owned_t s = _z_query_owned_null();
            z_result_t ret = _z_query_owned_copy(&s, _z_query_view_deref(_));
            if (ret != _Z_RES_OK) {
                *dst = _z_query_none();
                return ret;
            }
            *dst = _z_query_from_owned(&s);
        }),
        (none, *dst = _z_query_none())
    );
    return _Z_RES_OK;
}

void _z_query_create_view_from_data(_z_query_t *dst, const _z_keyexpr_t *key, const _z_value_t *value,
                                    const _z_slice_t *parameters, const _z_session_weak_t *zn,
                                    const _z_query_id_t *query_id, const _z_bytes_t *attachment, bool implicit_anyke,
                                    _z_n_qos_t qos, const _z_source_info_t *source_info) {
    dst->_view._inner._key._declaration = _z_keyexpr_wire_declaration_rc_null();
    dst->_view._inner._key._inner = *key;
    dst->_view._inner._value = value != NULL ? *value : _z_value_null();
    dst->_view._inner._attachment = attachment != NULL ? *attachment : _z_bytes_null();
    dst->_view._inner._parameters._slice = parameters != NULL ? *parameters : _z_slice_null();
    dst->_view._inner._id = *query_id;
    dst->_view._inner._anyke = implicit_anyke || _z_parameters_has_anyke(_z_string_data(&dst->_view._inner._parameters),
                                                                         _z_string_len(&dst->_view._inner._parameters));
    // a view on weak session: currently this references a weak pointer inside session's transport, which is not ideal,
    // if we ever change rc implementation to make rc = *(session, cnt) - it is safer to acquire it from here,
    dst->_view._inner._zn = *zn;
    dst->_view._inner._qos = qos;
    dst->_view._inner._source_info = source_info != NULL ? *source_info : _z_source_info_null();
    dst->_tag = _z_query_tag_view;
}

z_result_t _z_query_copy(_z_query_t *dst, const _z_query_t *src) {
    const _z_query_owned_t *ref = _z_query_get_ref(src);
    if (ref != NULL) {
        _z_query_owned_t s = _z_query_owned_null();
        z_result_t ret = _z_query_owned_copy(&s, ref);
        if (ret != _Z_RES_OK) {
            *dst = _z_query_none();
            return ret;
        }
        *dst = _z_query_from_owned(&s);
    } else {
        *dst = _z_query_none();
    }
    return _Z_RES_OK;
}

void _z_queryable_clear(_z_queryable_t *qbl) {
    _z_session_weak_drop(&qbl->_zn);
    _z_sync_group_drop(&qbl->_callback_drop_sync_group);
    *qbl = _z_queryable_null();
}

#endif
