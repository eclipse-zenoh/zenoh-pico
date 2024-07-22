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
void _z_declare_data_clear(_z_declare_data_t *data) { _z_keyexpr_clear(&data->_key); }

_Bool _z_declare_data_eq(const _z_declare_data_t *left, const _z_declare_data_t *right) {
    return ((left->_id == right->_id) && (left->_type == right->_type));
}

_Bool _z_session_interest_eq(const _z_session_interest_t *one, const _z_session_interest_t *two) {
    return one->_id == two->_id;
}

void _z_session_interest_clear(_z_session_interest_t *intr) { _z_keyexpr_clear(&intr->_key); }

/*------------------ interest ------------------*/
static _z_session_interest_rc_t *__z_get_interest_by_id(_z_session_interest_rc_list_t *intrs, const _z_zint_t id) {
    _z_session_interest_rc_t *ret = NULL;
    _z_session_interest_rc_list_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_list_head(xs);
        if (id == _Z_RC_IN_VAL(intr)->_id) {
            ret = intr;
            break;
        }
        xs = _z_session_interest_rc_list_tail(xs);
    }
    return ret;
}

static _z_session_interest_rc_list_t *__z_get_interest_by_key_and_flags(_z_session_interest_rc_list_t *intrs,
                                                                        uint8_t flags, const _z_keyexpr_t key) {
    _z_session_interest_rc_list_t *ret = NULL;
    _z_session_interest_rc_list_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_list_head(xs);
        if ((_Z_RC_IN_VAL(intr)->_flags & flags) != 0) {
            if (_z_keyexpr_intersects(_Z_RC_IN_VAL(intr)->_key._suffix, strlen(_Z_RC_IN_VAL(intr)->_key._suffix),
                                      key._suffix, strlen(key._suffix))) {
                ret = _z_session_interest_rc_list_push(ret, _z_session_interest_rc_clone_as_ptr(intr));
            }
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
static _z_session_interest_rc_t *__unsafe_z_get_interest_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_session_interest_rc_list_t *intrs = zn->_local_interests;
    return __z_get_interest_by_id(intrs, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static _z_session_interest_rc_list_t *__unsafe_z_get_interest_by_key_and_flags(_z_session_t *zn, uint8_t flags,
                                                                               const _z_keyexpr_t key) {
    _z_session_interest_rc_list_t *intrs = zn->_local_interests;
    return __z_get_interest_by_key_and_flags(intrs, flags, key);
}

static int8_t _z_interest_send_decl_resource(_z_session_t *zn, uint32_t interest_id) {
    _zp_session_lock_mutex(zn);
    _z_resource_list_t *res_list = _z_resource_list_clone(zn->_local_resources);
    _zp_session_unlock_mutex(zn);
    _z_resource_list_t *xs = res_list;
    while (xs != NULL) {
        _z_resource_t *res = _z_resource_list_head(xs);
        // Build the declare message to send on the wire
        _z_keyexpr_t key = _z_keyexpr_alias(res->_key);
        _z_declaration_t declaration = _z_make_decl_keyexpr(res->_id, &key);
        _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, true, interest_id);
        if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            return _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_n_msg_clear(&n_msg);
        xs = _z_resource_list_tail(xs);
    }
    _z_resource_list_free(&res_list);
    return _Z_RES_OK;
}

#if Z_FEATURE_SUBSCRIPTION == 1
static int8_t _z_interest_send_decl_subscriber(_z_session_t *zn, uint32_t interest_id) {
    _zp_session_lock_mutex(zn);
    _z_subscription_rc_list_t *sub_list = _z_subscription_rc_list_clone(zn->_local_subscriptions);
    _zp_session_unlock_mutex(zn);
    _z_subscription_rc_list_t *xs = sub_list;
    while (xs != NULL) {
        _z_subscription_rc_t *sub = _z_subscription_rc_list_head(xs);
        // Build the declare message to send on the wire
        _z_keyexpr_t key = _z_keyexpr_alias(_Z_RC_IN_VAL(sub)->_key);
        _z_declaration_t declaration = _z_make_decl_subscriber(
            &key, _Z_RC_IN_VAL(sub)->_id, _Z_RC_IN_VAL(sub)->_info.reliability == Z_RELIABILITY_RELIABLE);
        _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, true, interest_id);
        if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            return _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_n_msg_clear(&n_msg);
        xs = _z_subscription_rc_list_tail(xs);
    }
    _z_subscription_rc_list_free(&sub_list);
    return _Z_RES_OK;
}
#else
static int8_t _z_interest_send_decl_subscriber(_z_session_t *zn, uint32_t interest_id) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(interest_id);
    return _Z_RES_OK;
}
#endif

#if Z_FEATURE_QUERYABLE == 1
static int8_t _z_interest_send_decl_queryable(_z_session_t *zn, uint32_t interest_id) {
    _zp_session_lock_mutex(zn);
    _z_session_queryable_rc_list_t *qle_list = _z_session_queryable_rc_list_clone(zn->_local_queryable);
    _zp_session_unlock_mutex(zn);
    _z_session_queryable_rc_list_t *xs = qle_list;
    while (xs != NULL) {
        _z_session_queryable_rc_t *qle = _z_session_queryable_rc_list_head(xs);
        // Build the declare message to send on the wire
        _z_keyexpr_t key = _z_keyexpr_alias(_Z_RC_IN_VAL(qle)->_key);
        _z_declaration_t declaration = _z_make_decl_queryable(
            &key, _Z_RC_IN_VAL(qle)->_id, _Z_RC_IN_VAL(qle)->_complete, _Z_QUERYABLE_DISTANCE_DEFAULT);
        _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, true, interest_id);
        if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
            return _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_n_msg_clear(&n_msg);
        xs = _z_subscription_rc_list_tail(xs);
    }
    _z_session_queryable_rc_list_free(&qle_list);
    return _Z_RES_OK;
}
#else
static int8_t _z_interest_send_decl_queryable(_z_session_t *zn, uint32_t interest_id) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(interest_id);
    return _Z_RES_OK;
}
#endif

static int8_t _z_interest_send_declare_final(_z_session_t *zn, uint32_t interest_id) {
    _z_declaration_t decl = _z_make_decl_final();
    _z_network_message_t n_msg = _z_n_msg_make_declare(decl, true, interest_id);
    if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != _Z_RES_OK) {
        return _Z_ERR_TRANSPORT_TX_FAILED;
    }
    _z_n_msg_clear(&n_msg);
    return _Z_RES_OK;
}

_z_session_interest_rc_t *_z_get_interest_by_id(_z_session_t *zn, const _z_zint_t id) {
    _zp_session_lock_mutex(zn);
    _z_session_interest_rc_t *intr = __unsafe_z_get_interest_by_id(zn, id);
    _zp_session_unlock_mutex(zn);
    return intr;
}

_z_session_interest_rc_t *_z_register_interest(_z_session_t *zn, _z_session_interest_t *intr) {
    _Z_DEBUG(">>> Allocating interest for (%ju:%s)", (uintmax_t)intr->_key._id, intr->_key._suffix);
    _z_session_interest_rc_t *ret = NULL;

    _zp_session_lock_mutex(zn);
    ret = (_z_session_interest_rc_t *)z_malloc(sizeof(_z_session_interest_rc_t));
    if (ret != NULL) {
        *ret = _z_session_interest_rc_new_from_val(intr);
        zn->_local_interests = _z_session_interest_rc_list_push(zn->_local_interests, ret);
    }
    _zp_session_unlock_mutex(zn);
    return ret;
}

static int8_t _unsafe_z_register_declare(_z_session_t *zn, const _z_keyexpr_t *key, uint32_t id, uint8_t type) {
    _z_declare_data_t *decl = NULL;
    decl = (_z_declare_data_t *)z_malloc(sizeof(_z_declare_data_t));
    if (decl == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _z_keyexpr_copy(&decl->_key, key);
    decl->_id = id;
    decl->_type = type;
    zn->_remote_declares = _z_declare_data_list_push(zn->_remote_declares, decl);
    return _Z_RES_OK;
}

static _z_keyexpr_t _unsafe_z_get_key_from_declare(_z_session_t *zn, uint32_t id, uint8_t type) {
    _z_declare_data_list_t *xs = zn->_remote_declares;
    _z_declare_data_t comp = {
        ._key = _z_keyexpr_null(),
        ._id = id,
        ._type = type,
    };
    while (xs != NULL) {
        _z_declare_data_t *decl = _z_declare_data_list_head(xs);
        if (_z_declare_data_eq(&comp, decl)) {
            return _z_keyexpr_duplicate(decl->_key);
        }
        xs = _z_declare_data_list_tail(xs);
    }
    return _z_keyexpr_null();
}

static int8_t _unsafe_z_unregister_declare(_z_session_t *zn, uint32_t id, uint8_t type) {
    _z_declare_data_t decl = {
        ._key = _z_keyexpr_null(),
        ._id = id,
        ._type = type,
    };
    zn->_remote_declares = _z_declare_data_list_drop_filter(zn->_remote_declares, _z_declare_data_eq, &decl);
    return _Z_RES_OK;
}

int8_t _z_interest_process_declares(_z_session_t *zn, const _z_declaration_t *decl) {
    const _z_keyexpr_t *decl_key = NULL;
    _z_interest_msg_t msg;
    uint8_t flags = 0;
    uint8_t decl_type = 0;
    switch (decl->_tag) {
        case _Z_DECL_SUBSCRIBER:
            msg.type = _Z_INTEREST_MSG_TYPE_DECL_SUBSCRIBER;
            msg.id = decl->_body._decl_subscriber._id;
            decl_key = &decl->_body._decl_subscriber._keyexpr;
            decl_type = _Z_DECLARE_TYPE_SUBSCRIBER;
            flags = _Z_INTEREST_FLAG_SUBSCRIBERS;
            break;
        case _Z_DECL_QUERYABLE:
            msg.type = _Z_INTEREST_MSG_TYPE_DECL_QUERYABLE;
            msg.id = decl->_body._decl_queryable._id;
            decl_key = &decl->_body._decl_queryable._keyexpr;
            decl_type = _Z_DECLARE_TYPE_QUERYABLE;
            flags = _Z_INTEREST_FLAG_QUERYABLES;
            break;
        default:
            return _Z_ERR_MESSAGE_ZENOH_DECLARATION_UNKNOWN;
    }
    // Retrieve key
    _zp_session_lock_mutex(zn);
    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, decl_key);
    if (key._suffix == NULL) {
        _zp_session_unlock_mutex(zn);
        return _Z_ERR_KEYEXPR_UNKNOWN;
    }
    // Register declare
    _unsafe_z_register_declare(zn, &key, msg.id, decl_type);
    // Retrieve interests
    _z_session_interest_rc_list_t *intrs = __unsafe_z_get_interest_by_key_and_flags(zn, flags, key);
    _zp_session_unlock_mutex(zn);
    // Parse session_interest list
    _z_session_interest_rc_list_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_list_head(xs);
        if (_Z_RC_IN_VAL(intr)->_callback != NULL) {
            _Z_RC_IN_VAL(intr)->_callback(&msg, _Z_RC_IN_VAL(intr)->_arg);
        }
        xs = _z_session_interest_rc_list_tail(xs);
    }
    // Clean up
    _z_keyexpr_clear(&key);
    _z_session_interest_rc_list_free(&intrs);
    return _Z_RES_OK;
}

int8_t _z_interest_process_undeclares(_z_session_t *zn, const _z_declaration_t *decl) {
    _z_interest_msg_t msg;
    uint8_t flags = 0;
    uint8_t decl_type = 0;
    switch (decl->_tag) {
        case _Z_UNDECL_SUBSCRIBER:
            msg.type = _Z_INTEREST_MSG_TYPE_UNDECL_SUBSCRIBER;
            msg.id = decl->_body._undecl_subscriber._id;
            decl_type = _Z_DECLARE_TYPE_SUBSCRIBER;
            flags = _Z_INTEREST_FLAG_SUBSCRIBERS;
            break;
        case _Z_UNDECL_QUERYABLE:
            msg.type = _Z_INTEREST_MSG_TYPE_UNDECL_QUERYABLE;
            msg.id = decl->_body._undecl_queryable._id;
            decl_type = _Z_DECLARE_TYPE_QUERYABLE;
            flags = _Z_INTEREST_FLAG_QUERYABLES;
            break;
        default:
            return _Z_ERR_MESSAGE_ZENOH_DECLARATION_UNKNOWN;
    }
    _zp_session_lock_mutex(zn);
    // Retrieve declare data
    _z_keyexpr_t key = _unsafe_z_get_key_from_declare(zn, msg.id, decl_type);
    if (key._suffix == NULL) {
        _zp_session_unlock_mutex(zn);
        return _Z_ERR_KEYEXPR_UNKNOWN;
    }
    _z_session_interest_rc_list_t *intrs = __unsafe_z_get_interest_by_key_and_flags(zn, flags, key);
    // Remove declare
    _unsafe_z_unregister_declare(zn, msg.id, decl_type);
    _zp_session_unlock_mutex(zn);

    // Parse session_interest list
    _z_session_interest_rc_list_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_list_head(xs);
        if (_Z_RC_IN_VAL(intr)->_callback != NULL) {
            _Z_RC_IN_VAL(intr)->_callback(&msg, _Z_RC_IN_VAL(intr)->_arg);
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
    _z_declare_data_list_free(&zn->_remote_declares);
    _zp_session_unlock_mutex(zn);
}

int8_t _z_interest_process_declare_final(_z_session_t *zn, uint32_t id) {
    _z_interest_msg_t msg = {.type = _Z_INTEREST_MSG_TYPE_FINAL, .id = id};
    // Retrieve interest
    _zp_session_lock_mutex(zn);
    _z_session_interest_rc_t *intr = __unsafe_z_get_interest_by_id(zn, id);
    _zp_session_unlock_mutex(zn);
    if (intr == NULL) {
        return _Z_RES_OK;
    }
    // Trigger callback
    if (_Z_RC_IN_VAL(intr)->_callback != NULL) {
        _Z_RC_IN_VAL(intr)->_callback(&msg, _Z_RC_IN_VAL(intr)->_arg);
    }
    return _Z_RES_OK;
}

int8_t _z_interest_process_interest_final(_z_session_t *zn, uint32_t id) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(id);
    // TODO: Update future masks
    return _Z_RES_OK;
}

int8_t _z_interest_process_interest(_z_session_t *zn, _z_keyexpr_t key, uint32_t id, uint8_t flags) {
    // TODO: process restricted flag & associated key
    _ZP_UNUSED(key);
    // Check transport type
    if (zn->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        return _Z_RES_OK;  // Nothing to do on unicast
    }
    // Current flags process
    if (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_CURRENT)) {
        // Send all declare
        if (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_KEYEXPRS)) {
            _Z_DEBUG("Sending declare resources");
            _Z_RETURN_IF_ERR(_z_interest_send_decl_resource(zn, id));
        }
        if (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_SUBSCRIBERS)) {
            _Z_DEBUG("Sending declare subscribers");
            _Z_RETURN_IF_ERR(_z_interest_send_decl_subscriber(zn, id));
        }
        if (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_QUERYABLES)) {
            _Z_DEBUG("Sending declare queryables");
            _Z_RETURN_IF_ERR(_z_interest_send_decl_queryable(zn, id));
        }
        if (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_TOKENS)) {
            // Zenoh pico doesn't support liveliness token for now
        }
        // Send final declare
        _Z_RETURN_IF_ERR(_z_interest_send_declare_final(zn, id));
    }
    return _Z_RES_OK;
}

#else
void _z_flush_interest(_z_session_t *zn) { _ZP_UNUSED(zn); }

int8_t _z_interest_process_declares(_z_session_t *zn, const _z_declaration_t *decl) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(decl);
    return _Z_RES_OK;
}

int8_t _z_interest_process_undeclares(_z_session_t *zn, const _z_declaration_t *decl) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(decl);
    return _Z_RES_OK;
}

int8_t _z_interest_process_declare_final(_z_session_t *zn, uint32_t id) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(id);
    return _Z_RES_OK;
}

int8_t _z_interest_process_interest_final(_z_session_t *zn, uint32_t id) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(id);
    return _Z_RES_OK;
}

int8_t _z_interest_process_interest(_z_session_t *zn, _z_keyexpr_t key, uint32_t id, uint8_t flags) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(key);
    _ZP_UNUSED(id);
    _ZP_UNUSED(flags);
    return _Z_RES_OK;
}
#endif
