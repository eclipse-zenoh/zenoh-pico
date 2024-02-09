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
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/queryable.h"
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
    _zp_session_lock_mutex(zn);
    _z_session_interest_rc_t *intr = __unsafe_z_get_interest_by_id(zn, id);
    _zp_session_unlock_mutex(zn);
    return intr;
}

_z_session_interest_rc_list_t *_z_get_interest_by_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr) {
    _zp_session_lock_mutex(zn);
    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, keyexpr);
    _z_session_interest_rc_list_t *intrs = __unsafe_z_get_interest_by_key(zn, key);
    _zp_session_unlock_mutex(zn);
    return intrs;
}

_z_session_interest_rc_t *_z_register_interest(_z_session_t *zn, _z_session_interest_t *intr) {
    _Z_DEBUG(">>> Allocating interest for (%ju:%s)", (uintmax_t)intr->_key._id, intr->_key._suffix);
    _z_session_interest_rc_t *ret = NULL;

    _zp_session_lock_mutex(zn);
    ret = (_z_session_interest_rc_t *)zp_malloc(sizeof(_z_session_interest_rc_t));
    if (ret != NULL) {
        *ret = _z_session_interest_rc_new_from_val(*intr);
        zn->_local_interests = _z_session_interest_rc_list_push(zn->_local_interests, ret);
    }
    _zp_session_unlock_mutex(zn);
    return ret;
}

int8_t _z_trigger_interests(_z_session_t *zn, const _z_declaration_t *decl) {
    const _z_keyexpr_t *decl_key = NULL;
    _z_interest_msg_t msg;
    switch (decl->_tag) {
        case _Z_DECL_SUBSCRIBER:
            msg._type = _Z_INTEREST_MSG_TYPE_SUBSCRIBER;
            msg._id = decl->_body._decl_subscriber._id;
            decl_key = &decl->_body._decl_subscriber._keyexpr;
            break;
        case _Z_DECL_QUERYABLE:
            msg._type = _Z_INTEREST_MSG_TYPE_QUERYABLE;
            msg._id = decl->_body._decl_queryable._id;
            decl_key = &decl->_body._decl_queryable._keyexpr;
            break;
        default:
            return _Z_ERR_MESSAGE_ZENOH_DECLARATION_UNKNOWN;
    }
    _zp_session_lock_mutex(zn);
    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, decl_key);
    if (key._suffix != NULL) {
        _zp_session_unlock_mutex(zn);
        return _Z_ERR_KEYEXPR_UNKNOWN;
    }
    _z_session_interest_rc_list_t *intrs = __unsafe_z_get_interest_by_key(zn, key);
    _zp_session_unlock_mutex(zn);

    // Parse session_interest list
    _z_session_interest_rc_list_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_list_head(xs);
        if (intr->in->val._callback != NULL) {
            intr->in->val._callback(&msg, intr->in->val._arg);
        }
        xs = _z_session_interest_rc_list_tail(xs);
    }
    // Clean up
    _z_keyexpr_clear(&key);
    _z_session_interest_rc_list_free(&intrs);
    return _Z_RES_OK;
}

void _z_unregister_interest(_z_session_t *zn, _z_session_interest_rc_t *intr) {
    _zp_session_lock_mutex(zn);
    zn->_local_interests =
        _z_session_interest_rc_list_drop_filter(zn->_local_interests, _z_session_interest_rc_eq, intr);
    _zp_session_unlock_mutex(zn);
}

void _z_flush_interest(_z_session_t *zn) {
    _zp_session_lock_mutex(zn);
    _z_session_interest_rc_list_free(&zn->_local_interests);
    _zp_session_unlock_mutex(zn);
}

int8_t _z_process_final_interest(_z_session_t *zn, uint32_t id) {
    _z_interest_msg_t msg = {._type = _Z_INTEREST_MSG_TYPE_FINAL, ._id = id};
    // Retrieve interest
    _zp_session_lock_mutex(zn);
    _z_session_interest_rc_t *intr = __unsafe_z_get_interest_by_id(zn, id);
    _zp_session_unlock_mutex(zn);
    // Trigger callback
    if (intr->in->val._callback != NULL) {
        intr->in->val._callback(&msg, intr->in->val._arg);
    }
    return _Z_RES_OK;
}

int8_t _z_process_undeclare_interest(_z_session_t *zn, uint32_t id) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(id);
    // Update future masks
    return _Z_RES_OK;
}

static int8_t _z_send_resource_interest(_z_session_t *zn) {
    _z_resource_list_t *xs = zn->_local_resources;
    while (xs != NULL) {
        _z_resource_t *res = _z_resource_list_head(xs);
        // Build the declare message to send on the wire
        _z_declaration_t declaration = _z_make_decl_keyexpr(res->_id, &res->_key);
        _z_network_message_t n_msg = _z_n_msg_make_declare(declaration);
        if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            return _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_n_msg_clear(&n_msg);
        xs = _z_resource_list_tail(xs);
    }
    return _Z_RES_OK;
}

#if Z_FEATURE_SUBSCRIPTION == 1
static int8_t _z_send_subscriber_interest(_z_session_t *zn) {
    _z_subscription_rc_list_t *xs = zn->_local_subscriptions;
    while (xs != NULL) {
        _z_subscription_rc_t *sub = _z_subscription_rc_list_head(xs);
        // Build the declare message to send on the wire
        _z_declaration_t declaration = _z_make_decl_subscriber(&sub->in->val._key, sub->in->val._id,
                                                               sub->in->val._info.reliability == Z_RELIABILITY_RELIABLE,
                                                               sub->in->val._info.mode == Z_SUBMODE_PULL);
        _z_network_message_t n_msg = _z_n_msg_make_declare(declaration);
        if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            return _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_n_msg_clear(&n_msg);
        xs = _z_subscription_rc_list_tail(xs);
    }
    return _Z_RES_OK;
}
#else
static int8_t _z_send_subscriber_interest(_z_session_t *zn) {
    _ZP_UNUSED(zn);
    return _Z_RES_OK;
}
#endif

#if Z_FEATURE_QUERYABLE == 1
static int8_t _z_send_queryable_interest(_z_session_t *zn) {
    _z_session_queryable_rc_list_t *xs = zn->_local_queryable;
    while (xs != NULL) {
        _z_session_queryable_rc_t *qle = _z_session_queryable_rc_list_head(xs);
        // Build the declare message to send on the wire
        _z_declaration_t declaration = _z_make_decl_queryable(&qle->in->val._key, qle->in->val._id,
                                                              qle->in->val._complete, _Z_QUERYABLE_DISTANCE_DEFAULT);
        _z_network_message_t n_msg = _z_n_msg_make_declare(declaration);
        if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            return _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_n_msg_clear(&n_msg);
        xs = _z_subscription_rc_list_tail(xs);
    }
    return _Z_RES_OK;
}
#else
static int8_t _z_send_queryable_interest(_z_session_t *zn) {
    _ZP_UNUSED(zn);
    return _Z_RES_OK;
}
#endif

int8_t _z_process_declare_interest(_z_session_t *zn, _z_keyexpr_t key, uint32_t id, uint8_t flags) {
    _ZP_UNUSED(key);
    _ZP_UNUSED(id);
    // Check transport type
    if (zn->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        return _Z_RES_OK;  // Nothing to do on unicast
    }
    // Current flags process
    if ((flags & _Z_INTEREST_FLAG_CURRENT) != 0) {
        // Send all declare
        if ((flags & _Z_INTEREST_FLAG_KEYEXPRS) != 0) {
            _Z_DEBUG("Sending declare resources");
            _Z_RETURN_IF_ERR(_z_send_resource_interest(zn));
        }
        if ((flags & _Z_INTEREST_FLAG_SUBSCRIBERS) != 0) {
            _Z_DEBUG("Sending declare subscribers");
            _Z_RETURN_IF_ERR(_z_send_subscriber_interest(zn));
        }
        if ((flags & _Z_INTEREST_FLAG_QUERYABLES) != 0) {
            _Z_DEBUG("Sending declare queryables");
            _Z_RETURN_IF_ERR(_z_send_queryable_interest(zn));
        }
        if ((flags & _Z_INTEREST_FLAG_TOKENS) != 0) {
            // Zenoh pico doesn't support liveliness token for now
        }
    }

    return _Z_RES_OK;
}

#endif
