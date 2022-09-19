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

#include "zenoh-pico/config.h"
#include "zenoh-pico/net/resource.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

int _z_questionable_eq(const _z_questionable_t *one, const _z_questionable_t *two) { return one->_id == two->_id; }

void _z_questionable_clear(_z_questionable_t *qle) {
    if (qle->_dropper != NULL) qle->_dropper(qle->_arg);
    _z_keyexpr_clear(&qle->_key);
}

/*------------------ Queryable ------------------*/
_z_questionable_sptr_t *__z_get_questionable_by_id(_z_questionable_sptr_list_t *qles, const _z_zint_t id) {
    _z_questionable_sptr_t *qle = NULL;
    while (qles != NULL) {
        qle = _z_questionable_sptr_list_head(qles);
        if (id == qle->ptr->_id) return qle;

        qles = _z_questionable_sptr_list_tail(qles);
    }

    return qle;
}

_z_questionable_sptr_list_t *__z_get_questionable_by_key(_z_questionable_sptr_list_t *qles, const _z_keyexpr_t key) {
    _z_questionable_sptr_list_t *xs = NULL;
    while (qles != NULL) {
        _z_questionable_sptr_t *qle = _z_questionable_sptr_list_head(qles);
        if (_z_keyexpr_intersect(qle->ptr->_key._suffix, strlen(qle->ptr->_key._suffix), key._suffix,
                                 strlen(key._suffix))) {
            xs = _z_questionable_sptr_list_push(xs, _z_questionable_sptr_clone_as_ptr(qle));
        }

        qles = _z_questionable_sptr_list_tail(qles);
    }

    return xs;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_questionable_sptr_t *__unsafe_z_get_questionable_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_questionable_sptr_list_t *qles = zn->_local_questionable;
    return __z_get_questionable_by_id(qles, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_questionable_sptr_list_t *__unsafe_z_get_questionable_by_key(_z_session_t *zn, const _z_keyexpr_t key) {
    _z_questionable_sptr_list_t *qles = zn->_local_questionable;
    return __z_get_questionable_by_key(qles, key);
}

_z_questionable_sptr_t *_z_get_questionable_by_id(_z_session_t *zn, const _z_zint_t id) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_questionable_sptr_t *qle = __unsafe_z_get_questionable_by_id(zn, id);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return qle;
}

_z_questionable_sptr_list_t *_z_get_questionable_by_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_LOCAL, keyexpr);
    _z_questionable_sptr_list_t *qles = __unsafe_z_get_questionable_by_key(zn, key);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return qles;
}

int _z_register_questionable(_z_session_t *zn, _z_questionable_t *q) {
    _Z_DEBUG(">>> Allocating queryable for (%lu:%s)\n", q->_key._id, q->_key._suffix);

#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_questionable_sptr_t *qle = (_z_questionable_sptr_t *)z_malloc(sizeof(_z_questionable_sptr_t));
    *qle = _z_questionable_sptr_new(*q);
    zn->_local_questionable = _z_questionable_sptr_list_push(zn->_local_questionable, qle);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return 0;
}

int _z_trigger_queryables(_z_session_t *zn, const _z_msg_query_t *query) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_REMOTE, &query->_key);
    if (key._suffix == NULL) {
        goto ERR_1;
    }

    // This two step approach allows the lock to be released while in the user callback
    // Scoped because of the previous goto ERR
    {
        _z_questionable_sptr_list_t *qles = __unsafe_z_get_questionable_by_key(zn, key);
        _z_questionable_sptr_list_t *xs = qles;
        size_t len = _z_questionable_sptr_list_len(xs);
        _z_questionable_handler_t callbacks[len];
        void *callbacks_args[len];
        size_t i = 0;
        while (xs != NULL) {
            _z_questionable_sptr_t *qle = _z_questionable_sptr_list_head(xs);
            callbacks[i] = qle->ptr->_callback;
            callbacks_args[i++] = qle->ptr->_arg;
            xs = _z_questionable_sptr_list_tail(xs);
        }

#if Z_MULTI_THREAD == 1
        _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

        // Build the query
        z_query_t q;
        q._zn = zn;
        q._qid = query->_qid;
        q._key = key;
        q._parameters = query->_parameters;
        for (i = 0; i < len; i++) {
            callbacks[i](&q, callbacks_args[i]);
        }

        _z_keyexpr_clear(&key);
        _z_questionable_sptr_list_free(&qles);
    }

    // Send the final reply
    // Final flagged reply context does not encode the ZID
    _z_bytes_t zid;
    _z_bytes_reset(&zid);
    int is_final = 1;
    _z_reply_context_t *rctx = _z_msg_make_reply_context(query->_qid, zid, is_final);

    // Congestion control
    int can_be_dropped = 0;

    // Create the final reply
    _z_zenoh_message_t z_msg = _z_msg_make_unit(can_be_dropped);
    z_msg._reply_context = rctx;

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) < 0) {
        goto ERR_2;
    }
    _z_msg_clear(&z_msg);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return 0;

ERR_2:
    _z_msg_clear(&z_msg);

ERR_1:
#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1
    return -1;
}

void _z_unregister_questionable(_z_session_t *zn, _z_questionable_sptr_t *qle) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    zn->_local_questionable =
        _z_questionable_sptr_list_drop_filter(zn->_local_questionable, _z_questionable_sptr_eq, qle);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1
}

void _z_flush_questionables(_z_session_t *zn) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_questionable_sptr_list_free(&zn->_local_questionable);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1
}
