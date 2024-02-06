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

#if Z_FEATURE_INTEREST == 1
_Bool _z_session_interest_eq(const _z_session_interest_t *one, const _z_session_interest_t *two) {
    return one->_id == two->_id;
}

void _z_session_interest_clear(_z_session_interest_t *intr) {
    if (intr->_dropper != NULL) {
        intr->_dropper(intr->_arg);
    }
    _z_keyexpr_clear(&intr->_key);
}

/*------------------ interest ------------------*/
_z_session_interest_rc_t *__z_get_interest_by_id(_z_session_interest_rc_list_t *intrs, const _z_zint_t id) {
    _z_session_interest_rc_t *ret = NULL;

    _z_session_interest_rc_list_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_list_head(xs);
        if (id == intr->in->val._id) {
            ret = intr;
            break;
        }

        xs = _z_session_interest_rc_list_tail(xs);
    }

    return ret;
}

_z_session_interest_rc_list_t *__z_get_interest_by_key(_z_session_interest_rc_list_t *intrs, const _z_keyexpr_t key) {
    _z_session_interest_rc_list_t *ret = NULL;

    _z_session_interest_rc_list_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_list_head(xs);
        if (_z_keyexpr_intersects(intr->in->val._key._suffix, strlen(intr->in->val._key._suffix), key._suffix,
                                  strlen(key._suffix)) == true) {
            ret = _z_session_interest_rc_list_push(ret, _z_session_interest_rc_clone_as_ptr(intr));
        }

        xs = _z_session_interest_rc_list_tail(xs);
    }

    return ret;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_session_interest_rc_t *__unsafe_z_get_interest_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_session_interest_rc_list_t *intrs = zn->_local_interests;
    return __z_get_interest_by_id(intrs, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_session_interest_rc_list_t *__unsafe_z_get_interest_by_key(_z_session_t *zn, const _z_keyexpr_t key) {
    _z_session_interest_rc_list_t *intrs = zn->_local_interests;
    return __z_get_interest_by_key(intrs, key);
}

_z_session_interest_rc_t *_z_get_interest_by_id(_z_session_t *zn, const _z_zint_t id) {
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    _z_session_interest_rc_t *intr = __unsafe_z_get_interest_by_id(zn, id);

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return intr;
}

_z_session_interest_rc_list_t *_z_get_interest_by_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr) {
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, keyexpr);
    _z_session_interest_rc_list_t *intrs = __unsafe_z_get_interest_by_key(zn, key);

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return intrs;
}

_z_session_interest_rc_t *_z_register_interest(_z_session_t *zn, _z_session_interest_t *intr) {
    _Z_DEBUG(">>> Allocating interest for (%ju:%s)", (uintmax_t)intr->_key._id, intr->_key._suffix);
    _z_session_interest_rc_t *ret = NULL;

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    ret = (_z_session_interest_rc_t *)zp_malloc(sizeof(_z_session_interest_rc_t));
    if (ret != NULL) {
        *ret = _z_session_interest_rc_new_from_val(*intr);
        zn->_local_interests = _z_session_interest_rc_list_push(zn->_local_interests, ret);
    }

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return ret;
}

int8_t _z_trigger_interests(_z_session_t *zn, const _z_msg_query_t *msgq, const _z_keyexpr_t q_key, uint32_t qid) {
    int8_t ret = _Z_RES_OK;

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, &q_key);
    if (key._suffix != NULL) {
        _z_session_interest_rc_list_t *intrs = __unsafe_z_get_interest_by_key(zn, key);

#if Z_FEATURE_MULTI_THREAD == 1
        zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

        // Parse session_interest list
        _z_session_interest_rc_list_t *xs = intrs;
        while (xs != NULL) {
            _z_session_interest_rc_t *intr = _z_session_interest_rc_list_head(xs);
            if (intr->in->val._callback != NULL) {
                intr->in->val._callback(NULL, intr->in->val._arg);
            }
            xs = _z_session_interest_rc_list_tail(xs);
        }
        // Clean up
        _z_keyexpr_clear(&key);
        _z_session_interest_rc_list_free(&intrs);
    } else {
#if Z_FEATURE_MULTI_THREAD == 1
        zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

        ret = _Z_ERR_KEYEXPR_UNKNOWN;
    }

    return ret;
}

void _z_unregister_interest(_z_session_t *zn, _z_session_interest_rc_t *intr) {
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    zn->_local_interests =
        _z_session_interest_rc_list_drop_filter(zn->_local_interests, _z_session_interest_rc_eq, intr);

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1
}

void _z_flush_interest(_z_session_t *zn) {
#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_lock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    _z_session_interest_rc_list_free(&zn->_local_interests);

#if Z_FEATURE_MULTI_THREAD == 1
    zp_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1
}

#endif
