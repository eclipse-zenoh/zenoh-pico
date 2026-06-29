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
//

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/api/types.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/filtering.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/session/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/locality.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/string.h"

#if Z_FEATURE_QUERYABLE == 1

#define _Z_QLEINFOS_VEC_SIZE 4  // Arbitrary initial size

bool _z_session_queryable_eq(const _z_session_queryable_t *one, const _z_session_queryable_t *two) {
    return one->_id == two->_id;
}

void _z_session_queryable_clear(_z_session_queryable_t *qle) {
    if (qle->_dropper != NULL) {
        qle->_dropper(qle->_arg);
        qle->_dropper = NULL;
    }
    _z_declared_keyexpr_clear(&qle->_key);
    _z_sync_group_notifier_drop(&qle->_session_callback_drop_notifier);
    _z_sync_group_notifier_drop(&qle->_queryable_callback_drop_notifier);
}

/*------------------ Queryable ------------------*/
static _z_session_queryable_rc_t *__z_get_session_queryable_by_id(_z_session_queryable_rc_slist_t *qles,
                                                                  const _z_zint_t id) {
    _z_session_queryable_rc_t *ret = NULL;

    _z_session_queryable_rc_slist_t *xs = qles;
    while (xs != NULL) {
        _z_session_queryable_rc_t *qle = _z_session_queryable_rc_slist_value(xs);
        if (id == _Z_RC_IN_VAL(qle)->_id) {
            ret = qle;
            break;
        }
        xs = _z_session_queryable_rc_slist_next(xs);
    }

    return ret;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static _z_session_queryable_rc_t *__unsafe_z_get_session_queryable_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_session_queryable_rc_slist_t *qles = zn->_local_queryable;
    return __z_get_session_queryable_by_id(qles, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static z_result_t __unsafe_z_get_session_queryables_by_key(_z_session_t *zn, const _z_keyexpr_t *key, bool is_remote,
                                                           _z_session_queryable_rc_svec_t *qle_infos) {
    _z_session_queryable_rc_slist_t *qles = zn->_local_queryable;

    *qle_infos = _z_session_queryable_rc_svec_make(_Z_QLEINFOS_VEC_SIZE);
    _Z_RETURN_ERR_OOM_IF_TRUE(qle_infos->_val == NULL);
    _z_session_queryable_rc_slist_t *xs = qles;
    while (xs != NULL) {
        // Parse queryable list
        _z_session_queryable_rc_t *qle = _z_session_queryable_rc_slist_value(xs);
        const _z_session_queryable_t *qle_val = _Z_RC_IN_VAL(qle);
        bool origin_allowed = is_remote ? _z_locality_allows_remote(qle_val->_allowed_origin)
                                        : _z_locality_allows_local(qle_val->_allowed_origin);
        if (origin_allowed && _z_keyexpr_intersects(&qle_val->_key._inner, key)) {
            _z_session_queryable_rc_t qle_clone = _z_session_queryable_rc_clone(qle);
            _Z_CLEAN_RETURN_IF_ERR(_z_session_queryable_rc_svec_append(qle_infos, &qle_clone, false),
                                   _z_session_queryable_rc_svec_clear(qle_infos));
        }
        xs = _z_session_queryable_rc_slist_next(xs);
    }
    return _Z_RES_OK;
}

_z_session_queryable_rc_t _z_get_session_queryable_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_session_queryable_rc_t out = _z_session_queryable_rc_null();
    _z_session_mutex_lock(zn);

    _z_session_queryable_rc_t *qle = __unsafe_z_get_session_queryable_by_id(zn, id);
    if (qle != NULL) {
        out = _z_session_queryable_rc_clone(qle);
    }
    _z_session_mutex_unlock(zn);

    return out;
}

_z_session_queryable_rc_t _z_register_session_queryable(_z_session_t *zn, _z_session_queryable_t *q) {
    _Z_DEBUG(">>> Allocating queryable for (%.*s)", (int)_z_string_len(&q->_key._inner._keyexpr),
             _z_string_data(&q->_key._inner._keyexpr));

    _z_session_queryable_rc_t out = _z_session_queryable_rc_new_from_val(q);
    if (_Z_RC_IS_NULL(&out)) {
        return out;
    }
    if (_z_session_mutex_lock_if_open(zn) != _Z_RES_OK) {
        _Z_WARN("Failed to acquire session mutex for registering queryable - session is closed");
        _z_session_queryable_rc_drop(&out);
        *q = _z_session_queryable_null();
        return out;
    }
    zn->_local_queryable = _z_session_queryable_rc_slist_push_empty(zn->_local_queryable);
    _z_session_queryable_rc_t *ret = _z_session_queryable_rc_slist_value(zn->_local_queryable);
    *ret = _z_session_queryable_rc_clone(
        &out);  // immediately increase reference count to prevent eventual drop by concurrent session close
    _z_session_mutex_unlock(zn);

#if Z_FEATURE_LOCAL_QUERYABLE == 1
    if (!_Z_RC_IS_NULL(&out) && _z_locality_allows_local(q->_allowed_origin)) {
        _z_session_queryable_t *qle_val = _Z_RC_IN_VAL(&out);
        _z_write_filter_notify_queryable(zn, &qle_val->_key._inner, qle_val->_allowed_origin, qle_val->_complete, true);
    }
#endif

    return out;
}

static z_result_t __unsafe_z_session_register_new_received_query(_z_session_t *zn, const _z_query_id_t *query_id) {
    if (_z_rid_to_count_hmap_get(&zn->_received_queries_id_to_count, query_id) != NULL) {
        _Z_WARN("Received query with rid %zu, for peer %p, which already exists in session %p ", (size_t)query_id->rid,
                query_id->peer_id, (void *)zn);
        return _Z_ERR_INVALID;
    }
    _z_query_id_t qid = *query_id;
    if (_z_rid_to_count_hmap_insert(&zn->_received_queries_id_to_count, &qid, &(uint32_t){1}) ==
        _z_rid_to_count_hmap_end(&zn->_received_queries_id_to_count)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

z_result_t _z_trigger_queryables(_z_session_t *zn, const _z_keyexpr_t *keyexpr, const _z_msg_query_t *msgq,
                                 uint32_t qid, _z_n_qos_t qos, _z_transport_peer_common_t *peer) {
    _z_query_id_t query_id = {.rid = qid, .peer_id = (void *)peer};

    _z_session_queryable_rc_svec_t qles = _z_session_queryable_rc_svec_null();
    _Z_RETURN_IF_ERR(_z_session_mutex_lock_if_open(zn));
    _Z_CLEAN_RETURN_IF_ERR(__unsafe_z_get_session_queryables_by_key(zn, keyexpr, query_id.peer_id != NULL, &qles),
                           _z_session_mutex_unlock(zn));
    _Z_CLEAN_RETURN_IF_ERR(__unsafe_z_session_register_new_received_query(zn, &query_id), _z_session_mutex_unlock(zn);
                           _z_session_queryable_rc_svec_clear(&qles));
    _z_session_mutex_unlock(zn);

    // Check if there are queryables
    size_t qle_nb = _z_session_queryable_rc_svec_len(&qles);
    _Z_DEBUG("Triggering %ju queryables for key %.*s", (uintmax_t)qle_nb, (int)_z_string_len(&keyexpr->_keyexpr),
             _z_string_data(&keyexpr->_keyexpr));

    _z_query_t query;
    _z_query_create_view_from_data(
        &query, keyexpr, _z_value_view_deref(&msgq->_ext_value), _z_slice_view_deref(&msgq->_parameters), &zn->_weak,
        &query_id, _z_bytes_view_deref(&msgq->_ext_attachment), msgq->_implicit_anyke, qos, &msgq->_ext_info);

    for (size_t i = 0; i < qle_nb; i++) {
        _z_session_queryable_t *qle_info = _Z_RC_IN_VAL(_z_session_queryable_rc_svec_get(&qles, i));
        qle_info->_callback(&query, qle_info->_arg);
    }
    _z_session_queryable_rc_svec_clear(&qles);
    _z_received_query_count_decrease(
        zn, _z_query_get_ref(&query));  // should not fail, unless session is closed, which is fine, since in
                                        // this case response final will be inferred by remote peers.
    return _Z_RES_OK;
}

void _z_unregister_session_queryable(_z_session_t *zn, _z_session_queryable_rc_t *qle) {
#if Z_FEATURE_LOCAL_QUERYABLE == 1
    _z_session_queryable_t *qle_val = _Z_RC_IN_VAL(qle);
    _z_write_filter_notify_queryable(zn, &qle_val->_key._inner, qle_val->_allowed_origin, qle_val->_complete, false);
#endif
    _z_session_mutex_lock(zn);
    zn->_local_queryable =
        _z_session_queryable_rc_slist_drop_first_filter(zn->_local_queryable, _z_session_queryable_rc_eq, qle);
    _z_session_mutex_unlock(zn);
    _z_session_queryable_rc_drop(qle);
}

void _z_flush_session_queryable(_z_session_t *zn) {
    _z_session_queryable_rc_slist_t *queryables;
    _z_session_mutex_lock(zn);
    queryables = zn->_local_queryable;
    zn->_local_queryable = _z_session_queryable_rc_slist_new();
    _z_session_mutex_unlock(zn);
    _z_session_queryable_rc_slist_free(&queryables);
}

void _z_flush_received_queries(_z_session_t *zn) {
    _z_session_mutex_lock(zn);
    // no sending response final - peers will be notified automatically when the session is closed
    _z_rid_to_count_hmap_destroy(&zn->_received_queries_id_to_count);
    _z_session_mutex_unlock(zn);
}

#else  //  Z_FEATURE_QUERYABLE == 0

#endif  // Z_FEATURE_QUERYABLE == 1
