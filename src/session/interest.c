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
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/multicast/lease.h"
#include "zenoh-pico/utils/logging.h"

static z_result_t _z_interest_send_decl_resource(_z_session_t *zn, uint32_t interest_id, void *peer,
                                                 _z_keyexpr_t *restr_key) {
    _z_session_mutex_lock(zn);
    _z_resource_slist_t *res_list = _z_resource_slist_clone(zn->_local_resources);
    _z_session_mutex_unlock(zn);
    _z_resource_slist_t *xs = res_list;
    while (xs != NULL) {
        _z_resource_t *res = _z_resource_slist_value(xs);
        _z_keyexpr_t key = _z_keyexpr_alias(&res->_key);
        // Check if key is concerned
        if ((restr_key == NULL) || _z_keyexpr_suffix_intersects(restr_key, &key)) {
            // Build the declare message to send on the wire
            _z_declaration_t declaration = _z_make_decl_keyexpr(res->_id, &key);
            _z_network_message_t n_msg;
            _z_n_msg_make_declare(&n_msg, declaration, true, interest_id);
            if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK, peer) != _Z_RES_OK) {
                _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_TX_FAILED);
            }
            _z_n_msg_clear(&n_msg);
        }
        xs = _z_resource_slist_next(xs);
    }
    _z_resource_slist_free(&res_list);
    return _Z_RES_OK;
}

#if Z_FEATURE_SUBSCRIPTION == 1
static z_result_t _z_interest_send_decl_subscriber(_z_session_t *zn, uint32_t interest_id, void *peer,
                                                   _z_keyexpr_t *restr_key) {
    _z_session_mutex_lock(zn);
    _z_subscription_rc_slist_t *sub_list = _z_subscription_rc_slist_clone(zn->_subscriptions);
    _z_session_mutex_unlock(zn);
    _z_subscription_rc_slist_t *xs = sub_list;
    while (xs != NULL) {
        _z_subscription_rc_t *sub = _z_subscription_rc_slist_value(xs);
        // Check if key is concerned
        if ((restr_key == NULL) || _z_keyexpr_suffix_intersects(restr_key, &_Z_RC_IN_VAL(sub)->_key)) {
            // Build the declare message to send on the wire
            _z_keyexpr_t key = _z_keyexpr_alias(&_Z_RC_IN_VAL(sub)->_declared_key);
            _z_declaration_t declaration = _z_make_decl_subscriber(&key, _Z_RC_IN_VAL(sub)->_id);
            _z_network_message_t n_msg;
            _z_n_msg_make_declare(&n_msg, declaration, true, interest_id);
            if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK, peer) != _Z_RES_OK) {
                _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_TX_FAILED);
            }
            _z_n_msg_clear(&n_msg);
        }
        xs = _z_subscription_rc_slist_next(xs);
    }
    _z_subscription_rc_slist_free(&sub_list);
    return _Z_RES_OK;
}
#else
static z_result_t _z_interest_send_decl_subscriber(_z_session_t *zn, uint32_t interest_id, void *peer,
                                                   _z_keyexpr_t *restr_key) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(interest_id);
    _ZP_UNUSED(peer);
    _ZP_UNUSED(restr_key);
    return _Z_RES_OK;
}
#endif

#if Z_FEATURE_QUERYABLE == 1
static z_result_t _z_interest_send_decl_queryable(_z_session_t *zn, uint32_t interest_id, void *peer,
                                                  _z_keyexpr_t *restr_key) {
    _z_session_mutex_lock(zn);
    _z_session_queryable_rc_slist_t *qle_list = _z_session_queryable_rc_slist_clone(zn->_local_queryable);
    _z_session_mutex_unlock(zn);
    _z_session_queryable_rc_slist_t *xs = qle_list;
    while (xs != NULL) {
        _z_session_queryable_rc_t *qle = _z_session_queryable_rc_slist_value(xs);
        // Check if key is concerned
        if ((restr_key == NULL) || _z_keyexpr_suffix_intersects(restr_key, &_Z_RC_IN_VAL(qle)->_key)) {
            // Build the declare message to send on the wire
            _z_keyexpr_t key = _z_keyexpr_alias(&_Z_RC_IN_VAL(qle)->_declared_key);
            _z_declaration_t declaration = _z_make_decl_queryable(
                &key, _Z_RC_IN_VAL(qle)->_id, _Z_RC_IN_VAL(qle)->_complete, _Z_QUERYABLE_DISTANCE_DEFAULT);
            _z_network_message_t n_msg;
            _z_n_msg_make_declare(&n_msg, declaration, true, interest_id);
            if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK, peer) != _Z_RES_OK) {
                _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_TX_FAILED);
            }
            _z_n_msg_clear(&n_msg);
        }
        xs = _z_session_queryable_rc_slist_next(xs);
    }
    _z_session_queryable_rc_slist_free(&qle_list);
    return _Z_RES_OK;
}
#else
static z_result_t _z_interest_send_decl_queryable(_z_session_t *zn, uint32_t interest_id, void *peer,
                                                  _z_keyexpr_t *restr_key) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(interest_id);
    _ZP_UNUSED(peer);
    _ZP_UNUSED(restr_key);
    return _Z_RES_OK;
}
#endif

#if Z_FEATURE_LIVELINESS == 1
static z_result_t _z_interest_send_decl_token(_z_session_t *zn, uint32_t interest_id, void *peer,
                                              _z_keyexpr_t *restr_key) {
    _z_session_mutex_lock(zn);
    _z_keyexpr_intmap_t token_list = _z_keyexpr_intmap_clone(&zn->_local_tokens);
    _z_session_mutex_unlock(zn);
    _z_keyexpr_intmap_iterator_t iter = _z_keyexpr_intmap_iterator_make(&token_list);
    while (_z_keyexpr_intmap_iterator_next(&iter)) {
        uint32_t id = (uint32_t)_z_keyexpr_intmap_iterator_key(&iter);
        _z_keyexpr_t key = _z_keyexpr_alias(_z_keyexpr_intmap_iterator_value(&iter));
        // Check if key is concerned
        if ((restr_key == NULL) || _z_keyexpr_suffix_intersects(restr_key, &key)) {
            // Build the declare message to send on the wire
            _z_declaration_t declaration = _z_make_decl_token(&key, id);
            _z_network_message_t n_msg;
            _z_n_msg_make_declare(&n_msg, declaration, true, interest_id);
            if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK, peer) != _Z_RES_OK) {
                _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_TX_FAILED);
            }
            _z_n_msg_clear(&n_msg);
        }
    }
    _z_keyexpr_intmap_clear(&token_list);
    return _Z_RES_OK;
}
#else
static z_result_t _z_interest_send_decl_token(_z_session_t *zn, uint32_t interest_id, void *peer,
                                              _z_keyexpr_t *restr_key) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(interest_id);
    _ZP_UNUSED(peer);
    _ZP_UNUSED(restr_key);
    return _Z_RES_OK;
}
#endif

static z_result_t _z_interest_send_declare_final(_z_session_t *zn, uint32_t interest_id, void *peer) {
    _z_declaration_t decl = _z_make_decl_final();
    _z_network_message_t n_msg;
    _z_n_msg_make_declare(&n_msg, decl, true, interest_id);
    if (_z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK, peer) != _Z_RES_OK) {
        _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_TX_FAILED);
    }
    _z_n_msg_clear(&n_msg);
    return _Z_RES_OK;
}

z_result_t _z_interest_push_declarations_to_peer(_z_session_t *zn, void *peer) {
    _Z_RETURN_IF_ERR(_z_interest_send_decl_resource(zn, 0, peer, NULL));
    _Z_RETURN_IF_ERR(_z_interest_send_decl_subscriber(zn, 0, peer, NULL));
    _Z_RETURN_IF_ERR(_z_interest_send_decl_queryable(zn, 0, peer, NULL));
    _Z_RETURN_IF_ERR(_z_interest_send_decl_token(zn, 0, peer, NULL));
    _Z_RETURN_IF_ERR(_z_interest_send_declare_final(zn, 0, peer));
    return _Z_RES_OK;
}

z_result_t _z_interest_pull_resource_from_peers(_z_session_t *zn) {
    // Retrieve all current resource declarations
    uint32_t eid = _z_get_entity_id(zn);
    uint8_t flags = _Z_INTEREST_FLAG_KEYEXPRS | _Z_INTEREST_FLAG_CURRENT;
    // Send message on the network
    _z_interest_t interest = _z_make_interest(NULL, eid, flags);
    _z_network_message_t n_msg;
    _z_n_msg_make_interest(&n_msg, interest);
    z_result_t ret = _z_send_n_msg(zn, &n_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK, NULL);
    _z_n_msg_clear(&n_msg);
    return ret;
}

#if Z_FEATURE_INTEREST == 1
void _z_declare_data_clear(_z_declare_data_t *data) { _z_keyexpr_clear(&data->_key); }
size_t _z_declare_data_size(_z_declare_data_t *data) {
    _ZP_UNUSED(data);
    return sizeof(_z_declare_data_t);
}
void _z_declare_data_copy(_z_declare_data_t *dst, const _z_declare_data_t *src) {
    dst->_id = src->_id;
    dst->_type = src->_type;
    _z_keyexpr_copy(&dst->_key, &src->_key);
}

bool _z_declare_data_eq(const _z_declare_data_t *left, const _z_declare_data_t *right) {
    return ((left->_id == right->_id) && (left->_type == right->_type));
}

bool _z_session_interest_eq(const _z_session_interest_t *one, const _z_session_interest_t *two) {
    return one->_id == two->_id;
}

void _z_session_interest_clear(_z_session_interest_t *intr) { _z_keyexpr_clear(&intr->_key); }

/*------------------ interest ------------------*/
static _z_session_interest_rc_t *__z_get_interest_by_id(_z_session_interest_rc_slist_t *intrs, const _z_zint_t id) {
    _z_session_interest_rc_t *ret = NULL;
    _z_session_interest_rc_slist_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_slist_value(xs);
        if (id == _Z_RC_IN_VAL(intr)->_id) {
            ret = intr;
            break;
        }
        xs = _z_session_interest_rc_slist_next(xs);
    }
    return ret;
}

static _z_session_interest_rc_slist_t *__z_get_interest_by_key_and_flags(_z_session_interest_rc_slist_t *intrs,
                                                                         uint8_t flags, const _z_keyexpr_t *key) {
    _z_session_interest_rc_slist_t *ret = NULL;
    _z_session_interest_rc_slist_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_slist_value(xs);
        if ((_Z_RC_IN_VAL(intr)->_flags & flags) != 0) {
            if (_z_keyexpr_suffix_intersects(&_Z_RC_IN_VAL(intr)->_key, key)) {
                ret = _z_session_interest_rc_slist_push_empty(ret);
                _z_session_interest_rc_t *new_intr = _z_session_interest_rc_slist_value(ret);
                *new_intr = _z_session_interest_rc_clone(intr);
            }
        }
        xs = _z_session_interest_rc_slist_next(xs);
    }
    return ret;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static _z_session_interest_rc_t *__unsafe_z_get_interest_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_session_interest_rc_slist_t *intrs = zn->_local_interests;
    return __z_get_interest_by_id(intrs, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static _z_session_interest_rc_slist_t *__unsafe_z_get_interest_by_key_and_flags(_z_session_t *zn, uint8_t flags,
                                                                                const _z_keyexpr_t *key) {
    _z_session_interest_rc_slist_t *intrs = zn->_local_interests;
    return __z_get_interest_by_key_and_flags(intrs, flags, key);
}

_z_session_interest_rc_t *_z_get_interest_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_session_mutex_lock(zn);
    _z_session_interest_rc_t *intr = __unsafe_z_get_interest_by_id(zn, id);
    _z_session_mutex_unlock(zn);
    return intr;
}

_z_session_interest_rc_t *_z_register_interest(_z_session_t *zn, _z_session_interest_t *intr) {
    _Z_DEBUG(">>> Allocating interest for (%ju:%.*s)", (uintmax_t)intr->_key._id,
             (int)_z_string_len(&intr->_key._suffix), _z_string_data(&intr->_key._suffix));
    _z_session_interest_rc_t *ret = NULL;

    _z_session_mutex_lock(zn);
    zn->_local_interests = _z_session_interest_rc_slist_push_empty(zn->_local_interests);
    ret = _z_session_interest_rc_slist_value(zn->_local_interests);
    *ret = _z_session_interest_rc_new_from_val(intr);
    _z_session_mutex_unlock(zn);
    return ret;
}

static z_result_t _unsafe_z_register_declare(_z_session_t *zn, const _z_keyexpr_t *key, uint32_t id, uint8_t type) {
    zn->_remote_declares = _z_declare_data_slist_push_empty(zn->_remote_declares);
    _z_declare_data_t *decl = _z_declare_data_slist_value(zn->_remote_declares);
    _z_keyexpr_copy(&decl->_key, key);
    decl->_id = id;
    decl->_type = type;
    return _Z_RES_OK;
}

static _z_keyexpr_t _unsafe_z_get_key_from_declare(_z_session_t *zn, uint32_t id, uint8_t type) {
    _z_declare_data_slist_t *xs = zn->_remote_declares;
    _z_declare_data_t comp = {
        ._key = _z_keyexpr_null(),
        ._id = id,
        ._type = type,
    };
    while (xs != NULL) {
        _z_declare_data_t *decl = _z_declare_data_slist_value(xs);
        if (_z_declare_data_eq(&comp, decl)) {
            return _z_keyexpr_duplicate(&decl->_key);
        }
        xs = _z_declare_data_slist_next(xs);
    }
    return _z_keyexpr_null();
}

static z_result_t _unsafe_z_unregister_declare(_z_session_t *zn, uint32_t id, uint8_t type) {
    _z_declare_data_t decl = {
        ._key = _z_keyexpr_null(),
        ._id = id,
        ._type = type,
    };
    zn->_remote_declares = _z_declare_data_slist_drop_filter(zn->_remote_declares, _z_declare_data_eq, &decl);
    return _Z_RES_OK;
}

z_result_t _z_interest_process_declares(_z_session_t *zn, const _z_declaration_t *decl,
                                        _z_transport_peer_common_t *peer) {
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
        case _Z_DECL_TOKEN:
            msg.type = _Z_INTEREST_MSG_TYPE_DECL_TOKEN;
            msg.id = decl->_body._decl_token._id;
            decl_key = &decl->_body._decl_token._keyexpr;
            decl_type = _Z_DECLARE_TYPE_TOKEN;
            flags = _Z_INTEREST_FLAG_TOKENS;
            break;
        default:
            _Z_ERROR_RETURN(_Z_ERR_MESSAGE_ZENOH_DECLARATION_UNKNOWN);
    }
    // Retrieve key
    _z_session_mutex_lock(zn);
    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, decl_key, true, peer);
    if (!_z_keyexpr_has_suffix(&key)) {
        _z_session_mutex_unlock(zn);
        _Z_ERROR_RETURN(_Z_ERR_KEYEXPR_UNKNOWN);
    }
    // Register declare
    _unsafe_z_register_declare(zn, &key, msg.id, decl_type);
    // Retrieve interests
    _z_session_interest_rc_slist_t *intrs = __unsafe_z_get_interest_by_key_and_flags(zn, flags, &key);
    _z_session_mutex_unlock(zn);
    // Parse session_interest list
    _z_session_interest_rc_slist_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_slist_value(xs);
        if (_Z_RC_IN_VAL(intr)->_callback != NULL) {
            _Z_RC_IN_VAL(intr)->_callback(&msg, peer, _Z_RC_IN_VAL(intr)->_arg);
        }
        xs = _z_session_interest_rc_slist_next(xs);
    }
    // Clean up
    _z_keyexpr_clear(&key);
    _z_session_interest_rc_slist_free(&intrs);
    return _Z_RES_OK;
}

z_result_t _z_interest_process_undeclares(_z_session_t *zn, const _z_declaration_t *decl,
                                          _z_transport_peer_common_t *peer) {
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
        case _Z_UNDECL_TOKEN:
            msg.type = _Z_INTEREST_MSG_TYPE_UNDECL_TOKEN;
            msg.id = decl->_body._undecl_token._id;
            decl_type = _Z_DECLARE_TYPE_TOKEN;
            flags = _Z_INTEREST_FLAG_TOKENS;
            break;
        default:
            _Z_ERROR_RETURN(_Z_ERR_MESSAGE_ZENOH_DECLARATION_UNKNOWN);
    }
    _z_session_mutex_lock(zn);
    // Retrieve declare data
    _z_keyexpr_t key = _unsafe_z_get_key_from_declare(zn, msg.id, decl_type);
    if (!_z_keyexpr_has_suffix(&key)) {
        _z_session_mutex_unlock(zn);
        _Z_ERROR_RETURN(_Z_ERR_KEYEXPR_UNKNOWN);
    }
    _z_session_interest_rc_slist_t *intrs = __unsafe_z_get_interest_by_key_and_flags(zn, flags, &key);
    // Remove declare
    _unsafe_z_unregister_declare(zn, msg.id, decl_type);
    _z_session_mutex_unlock(zn);

    // Parse session_interest list
    _z_session_interest_rc_slist_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_slist_value(xs);
        if (_Z_RC_IN_VAL(intr)->_callback != NULL) {
            _Z_RC_IN_VAL(intr)->_callback(&msg, peer, _Z_RC_IN_VAL(intr)->_arg);
        }
        xs = _z_session_interest_rc_slist_next(xs);
    }
    // Clean up
    _z_keyexpr_clear(&key);
    _z_session_interest_rc_slist_free(&intrs);
    return _Z_RES_OK;
}

void _z_unregister_interest(_z_session_t *zn, _z_session_interest_rc_t *intr) {
    _z_session_mutex_lock(zn);
    zn->_local_interests =
        _z_session_interest_rc_slist_drop_filter(zn->_local_interests, _z_session_interest_rc_eq, intr);
    _z_session_mutex_unlock(zn);
}

void _z_interest_init(_z_session_t *zn) {
    _z_session_mutex_lock(zn);
    zn->_local_interests = NULL;
    zn->_remote_declares = NULL;
    _z_session_mutex_unlock(zn);
}

void _z_flush_interest(_z_session_t *zn) {
    _z_session_mutex_lock(zn);
    _z_session_interest_rc_slist_free(&zn->_local_interests);
    _z_declare_data_slist_free(&zn->_remote_declares);
    _z_session_mutex_unlock(zn);
}

z_result_t _z_interest_process_declare_final(_z_session_t *zn, uint32_t id, _z_transport_peer_common_t *peer) {
    _z_interest_msg_t msg = {.type = _Z_INTEREST_MSG_TYPE_FINAL, .id = id};
    // Retrieve interest
    _z_session_mutex_lock(zn);
    _z_session_interest_rc_t *intr = __unsafe_z_get_interest_by_id(zn, id);
    _z_session_mutex_unlock(zn);
    if (intr == NULL) {
        return _Z_RES_OK;
    }
    // Trigger callback
    if (_Z_RC_IN_VAL(intr)->_callback != NULL) {
        _Z_RC_IN_VAL(intr)->_callback(&msg, peer, _Z_RC_IN_VAL(intr)->_arg);
    }
    return _Z_RES_OK;
}

z_result_t _z_interest_process_interest_final(_z_session_t *zn, uint32_t id) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(id);
    // TODO: Update future masks
    return _Z_RES_OK;
}

z_result_t _z_interest_process_interest(_z_session_t *zn, _z_keyexpr_t *key, uint32_t id, uint8_t flags) {
    // Check transport type
    if (zn->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        return _Z_RES_OK;  // Nothing to do on unicast
    }
    // Push a join in case it's a new node
    _Z_RETURN_IF_ERR(_zp_multicast_send_join(&zn->_tp._transport._multicast));
    _z_keyexpr_t *restr_key = (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_RESTRICTED)) ? key : NULL;
    // Current flags process
    if (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_CURRENT)) {
        // Send all declare
        if (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_KEYEXPRS)) {
            _Z_DEBUG("Sending declare resources");
            _Z_RETURN_IF_ERR(_z_interest_send_decl_resource(zn, id, NULL, restr_key));
        }
        if (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_SUBSCRIBERS)) {
            _Z_DEBUG("Sending declare subscribers");
            _Z_RETURN_IF_ERR(_z_interest_send_decl_subscriber(zn, id, NULL, restr_key));
        }
        if (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_QUERYABLES)) {
            _Z_DEBUG("Sending declare queryables");
            _Z_RETURN_IF_ERR(_z_interest_send_decl_queryable(zn, id, NULL, restr_key));
        }
        if (_Z_HAS_FLAG(flags, _Z_INTEREST_FLAG_TOKENS)) {
            _Z_DEBUG("Sending declare tokens");
            _Z_RETURN_IF_ERR(_z_interest_send_decl_token(zn, id, NULL, restr_key));
        }
        // Send final declare
        _Z_RETURN_IF_ERR(_z_interest_send_declare_final(zn, id, NULL));
    }
    return _Z_RES_OK;
}

void _z_interest_peer_disconnected(_z_session_t *zn, _z_transport_peer_common_t *peer) {
    // Clone session interest list
    _z_session_mutex_lock(zn);
    _z_session_interest_rc_slist_t *intrs = _z_session_interest_rc_slist_clone(zn->_local_interests);
    _z_session_mutex_unlock(zn);

    // Parse session_interest list
    _z_interest_msg_t msg = {.id = 0, .type = _Z_INTEREST_MSG_TYPE_CONNECTION_DROPPED};
    _z_session_interest_rc_slist_t *xs = intrs;
    while (xs != NULL) {
        _z_session_interest_rc_t *intr = _z_session_interest_rc_slist_value(xs);
        if (_Z_RC_IN_VAL(intr)->_callback != NULL) {
            _Z_RC_IN_VAL(intr)->_callback(&msg, peer, _Z_RC_IN_VAL(intr)->_arg);
        }
        xs = _z_session_interest_rc_slist_next(xs);
    }
    // Clean up
    _z_session_interest_rc_slist_free(&intrs);
}

void _z_interest_replay_declare(_z_session_t *zn, _z_session_interest_t *interest) {
    _z_session_mutex_lock(zn);
    _z_declare_data_slist_t *res_list = _z_declare_data_slist_clone(zn->_remote_declares);
    _z_session_mutex_unlock(zn);

    _z_declare_data_slist_t *xs = res_list;
    while (xs != NULL) {
        _z_declare_data_t *res = _z_declare_data_slist_value(xs);
        if (_z_keyexpr_suffix_intersects(&interest->_key, &res->_key)) {
            _z_interest_msg_t msg = {0};
            switch (res->_type) {
                default:
                    break;
                case _Z_DECLARE_TYPE_QUERYABLE:
                    msg.type = _Z_INTEREST_MSG_TYPE_DECL_QUERYABLE;
                    break;
                case _Z_DECLARE_TYPE_SUBSCRIBER:
                    msg.type = _Z_INTEREST_MSG_TYPE_DECL_SUBSCRIBER;
                    break;
                case _Z_DECLARE_TYPE_TOKEN:
                    msg.type = _Z_INTEREST_MSG_TYPE_DECL_TOKEN;
                    break;
            }
            interest->_callback(&msg, (_z_transport_peer_common_t *)res->_key._mapping, interest->_arg);
        }
        xs = _z_declare_data_slist_next(xs);
    }
    _z_declare_data_slist_free(&res_list);
}

#else
void _z_interest_init(_z_session_t *zn) { _ZP_UNUSED(zn); }

void _z_flush_interest(_z_session_t *zn) { _ZP_UNUSED(zn); }

z_result_t _z_interest_process_declares(_z_session_t *zn, const _z_declaration_t *decl,
                                        _z_transport_peer_common_t *peer) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(decl);
    _ZP_UNUSED(peer);
    return _Z_RES_OK;
}

z_result_t _z_interest_process_undeclares(_z_session_t *zn, const _z_declaration_t *decl,
                                          _z_transport_peer_common_t *peer) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(decl);
    _ZP_UNUSED(peer);
    return _Z_RES_OK;
}

z_result_t _z_interest_process_declare_final(_z_session_t *zn, uint32_t id, _z_transport_peer_common_t *peer) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(id);
    _ZP_UNUSED(peer);
    return _Z_RES_OK;
}

z_result_t _z_interest_process_interest_final(_z_session_t *zn, uint32_t id) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(id);
    return _Z_RES_OK;
}

z_result_t _z_interest_process_interest(_z_session_t *zn, _z_keyexpr_t key, uint32_t id, uint8_t flags) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(key);
    _ZP_UNUSED(id);
    _ZP_UNUSED(flags);
    return _Z_RES_OK;
}

void _z_interest_peer_disconnected(_z_session_t *zn, _z_transport_peer_common_t *peer) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(peer);
}
#endif
