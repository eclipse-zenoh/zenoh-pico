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
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_QUERYABLE == 1
_Bool _z_questionable_eq(const _z_questionable_t *one, const _z_questionable_t *two) { return one->_id == two->_id; }

void _z_questionable_clear(_z_questionable_t *qle) {
    if (qle->_dropper != NULL) {
        qle->_dropper(qle->_arg);
    }
    _z_keyexpr_clear(&qle->_key);
}

/*------------------ Queryable ------------------*/
_z_questionable_rc_t *__z_get_questionable_by_id(_z_questionable_rc_list_t *qles, const _z_zint_t id) {
    _z_questionable_rc_t *ret = NULL;

    _z_questionable_rc_list_t *xs = qles;
    while (xs != NULL) {
        _z_questionable_rc_t *qle = _z_questionable_rc_list_head(xs);
        if (id == qle->in->val._id) {
            ret = qle;
            break;
        }

        xs = _z_questionable_rc_list_tail(xs);
    }

    return ret;
}

_z_questionable_rc_list_t *__z_get_questionable_by_key(_z_questionable_rc_list_t *qles, const _z_keyexpr_t key) {
    _z_questionable_rc_list_t *ret = NULL;

    _z_questionable_rc_list_t *xs = qles;
    while (xs != NULL) {
        _z_questionable_rc_t *qle = _z_questionable_rc_list_head(xs);
        if (_z_keyexpr_intersects(qle->in->val._key._suffix, strlen(qle->in->val._key._suffix), key._suffix,
                                  strlen(key._suffix)) == true) {
            ret = _z_questionable_rc_list_push(ret, _z_questionable_rc_clone_as_ptr(qle));
        }

        xs = _z_questionable_rc_list_tail(xs);
    }

    return ret;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_questionable_rc_t *__unsafe_z_get_questionable_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_questionable_rc_list_t *qles = zn->_local_questionable;
    return __z_get_questionable_by_id(qles, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_questionable_rc_list_t *__unsafe_z_get_questionable_by_key(_z_session_t *zn, const _z_keyexpr_t key) {
    _z_questionable_rc_list_t *qles = zn->_local_questionable;
    return __z_get_questionable_by_key(qles, key);
}

_z_questionable_rc_t *_z_get_questionable_by_id(_z_session_t *zn, const _z_zint_t id) {
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    _z_questionable_rc_t *qle = __unsafe_z_get_questionable_by_id(zn, id);

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return qle;
}

_z_questionable_rc_list_t *_z_get_questionable_by_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr) {
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, keyexpr);
    _z_questionable_rc_list_t *qles = __unsafe_z_get_questionable_by_key(zn, key);

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return qles;
}

_z_questionable_rc_t *_z_register_questionable(_z_session_t *zn, _z_questionable_t *q) {
    _Z_DEBUG(">>> Allocating queryable for (%ju:%s)", (uintmax_t)q->_key._id, q->_key._suffix);
    _z_questionable_rc_t *ret = NULL;

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    ret = (_z_questionable_rc_t *)zp_malloc(sizeof(_z_questionable_rc_t));
    if (ret != NULL) {
        *ret = _z_questionable_rc_new_from_val(*q);
        zn->_local_questionable = _z_questionable_rc_list_push(zn->_local_questionable, ret);
    }

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return ret;
}

int8_t _z_trigger_queryables(_z_session_t *zn, const _z_msg_query_t *msgq, const _z_keyexpr_t q_key, uint32_t qid) {
    int8_t ret = _Z_RES_OK;

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, &q_key);
    if (key._suffix != NULL) {
        _z_questionable_rc_list_t *qles = __unsafe_z_get_questionable_by_key(zn, key);

#if Z_FEATURE_MULTI_THREAD == 1
        zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

        // Build the z_query
        z_query_t query = {._val = {._rc = _z_query_rc_new()}};
        query._val._rc.in->val = _z_query_create(&msgq->_ext_value, &key, &msgq->_parameters, zn, qid);
        // Parse questionable list
        _z_questionable_rc_list_t *xs = qles;
        while (xs != NULL) {
            _z_questionable_rc_t *qle = _z_questionable_rc_list_head(xs);
            qle->in->val._callback(&query, qle->in->val._arg);
            xs = _z_questionable_rc_list_tail(xs);
        }
        // Clean up
        _z_query_rc_drop(&query._val._rc);
        _z_keyexpr_clear(&key);
        _z_questionable_rc_list_free(&qles);
    } else {
#if Z_FEATURE_MULTI_THREAD == 1
        zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

        ret = _Z_ERR_KEYEXPR_UNKNOWN;
    }

    return ret;
}

void _z_unregister_questionable(_z_session_t *zn, _z_questionable_rc_t *qle) {
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    zn->_local_questionable = _z_questionable_rc_list_drop_filter(zn->_local_questionable, _z_questionable_rc_eq, qle);

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1
}

void _z_flush_questionables(_z_session_t *zn) {
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    _z_questionable_rc_list_free(&zn->_local_questionable);

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1
}

#endif
