//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/api/liveliness.h"

#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/interest.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_LIVELINESS == 1

/**************** Liveliness Token ****************/

z_result_t _z_declare_liveliness_token(const _z_session_rc_t *zn, _z_liveliness_token_t *ret_token,
                                       _z_keyexpr_t *keyexpr) {
    z_result_t ret;

    uint32_t id = _z_get_entity_id(_Z_RC_IN_VAL(zn));

    _z_keyexpr_t ke = _z_keyexpr_duplicate(keyexpr);
    _z_declaration_t declaration = _z_make_decl_token(&ke, id);
    _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, false, 0);
    ret = _z_send_declare(_Z_RC_IN_VAL(zn), &n_msg);
    _z_n_msg_clear(&n_msg);

    _z_liveliness_register_token(_Z_RC_IN_VAL(zn), id, keyexpr);

    ret_token->_id = id;
    ret_token->_key = _z_keyexpr_steal(keyexpr);
    ret_token->_zn = _z_session_rc_clone_as_weak(zn);
    return ret;
}

z_result_t _z_undeclare_liveliness_token(_z_liveliness_token_t *token) {
    if (token == NULL || _Z_RC_IS_NULL(&token->_zn)) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }

    z_result_t ret;

    _z_liveliness_unregister_token(_Z_RC_IN_VAL(&token->_zn), token->_id);

    _z_declaration_t declaration = _z_make_undecl_token(token->_id, &token->_key);
    _z_network_message_t n_msg = _z_n_msg_make_declare(declaration, false, 0);
    ret = _z_send_undeclare(_Z_RC_IN_VAL(&token->_zn), &n_msg);
    _z_n_msg_clear(&n_msg);

    return ret;
}

/**************** Liveliness Subscriber ****************/

#if Z_FEATURE_SUBSCRIPTION == 1
_z_subscriber_t _z_declare_liveliness_subscriber(const _z_session_rc_t *zn, _z_keyexpr_t *keyexpr,
                                                 _z_closure_sample_callback_t callback, _z_drop_handler_t dropper,
                                                 bool history, void *arg) {
    _z_subscription_t s;
    s._id = _z_get_entity_id(_Z_RC_IN_VAL(zn));
    s._key_id = keyexpr->_id;
    s._key = _z_get_expanded_key_from_key(_Z_RC_IN_VAL(zn), keyexpr);
    s._callback = callback;
    s._dropper = dropper;
    s._arg = arg;

    _z_subscriber_t ret = _z_subscriber_null();
    // Register subscription, stored at session-level, do not drop it by the end of this function.
    _z_subscription_rc_t *sp_s =
        _z_register_subscription(_Z_RC_IN_VAL(zn), _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, &s);
    if (sp_s == NULL) {
        _z_subscriber_clear(&ret);
        return ret;
    }
    // Build the declare message to send on the wire
    uint8_t mode = history ? (_Z_INTEREST_FLAG_CURRENT | _Z_INTEREST_FLAG_FUTURE) : _Z_INTEREST_FLAG_FUTURE;
    _z_interest_t interest = _z_make_interest(
        keyexpr, s._id, _Z_INTEREST_FLAG_KEYEXPRS | _Z_INTEREST_FLAG_TOKENS | _Z_INTEREST_FLAG_RESTRICTED | mode);

    _z_network_message_t n_msg = _z_n_msg_make_interest(interest);
    if (_z_send_declare(_Z_RC_IN_VAL(zn), &n_msg) != _Z_RES_OK) {
        _z_unregister_subscription(_Z_RC_IN_VAL(zn), _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, sp_s);
        _z_subscriber_clear(&ret);
        return ret;
    }
    _z_n_msg_clear(&n_msg);

    ret._entity_id = s._id;
    ret._zn = _z_session_rc_clone_as_weak(zn);
    return ret;
}

z_result_t _z_undeclare_liveliness_subscriber(_z_subscriber_t *sub) {
    if (sub == NULL || _Z_RC_IS_NULL(&sub->_zn)) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }

    _z_subscription_rc_t *s =
        _z_get_subscription_by_id(_Z_RC_IN_VAL(&sub->_zn), _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, sub->_entity_id);
    if (s == NULL) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }

    _z_interest_t interest = _z_make_interest_final(s->_val->_id);
    _z_network_message_t n_msg = _z_n_msg_make_interest(interest);
    if (_z_send_undeclare(_Z_RC_IN_VAL(&sub->_zn), &n_msg) != _Z_RES_OK) {
        return _Z_ERR_TRANSPORT_TX_FAILED;
    }
    _z_n_msg_clear(&n_msg);

    _z_undeclare_resource(_Z_RC_IN_VAL(&sub->_zn), _Z_RC_IN_VAL(s)->_key_id);
    _z_unregister_subscription(_Z_RC_IN_VAL(&sub->_zn), _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, s);
    return _Z_RES_OK;
}
#endif  // Z_FEATURE_SUBSCRIPTION == 1

/**************** Liveliness Query ****************/

#if Z_FEATURE_QUERY == 1
z_result_t _z_liveliness_query(_z_session_t *zn, const _z_keyexpr_t *keyexpr, _z_closure_reply_callback_t callback,
                               _z_drop_handler_t dropper, void *arg, uint64_t timeout_ms) {
    z_result_t ret = _Z_RES_OK;

    // Create the pending liveliness query object
    _z_liveliness_pending_query_t *pq =
        (_z_liveliness_pending_query_t *)z_malloc(sizeof(_z_liveliness_pending_query_t));
    if (pq != NULL) {
        uint32_t id = _z_liveliness_get_query_id(zn);
        pq->_key = _z_get_expanded_key_from_key(zn, keyexpr);
        pq->_callback = callback;
        pq->_dropper = dropper;
        pq->_arg = arg;

        ret = _z_liveliness_register_pending_query(zn, id, pq);
        if (ret == _Z_RES_OK) {
            _ZP_UNUSED(timeout_ms);  // Current interest in pico don't support timeout
            _z_keyexpr_t key = _z_keyexpr_alias(keyexpr);
            _z_interest_t interest = _z_make_interest(&key, id,
                                                      _Z_INTEREST_FLAG_KEYEXPRS | _Z_INTEREST_FLAG_TOKENS |
                                                          _Z_INTEREST_FLAG_RESTRICTED | _Z_INTEREST_FLAG_CURRENT);

            _z_network_message_t n_msg = _z_n_msg_make_interest(interest);
            if (_z_send_declare(zn, &n_msg) != _Z_RES_OK) {
                _z_liveliness_unregister_pending_query(zn, id);
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
            }

            _z_n_msg_clear(&n_msg);

        } else {
            _z_liveliness_pending_query_clear(pq);
            z_free(pq);
        }
    }

    return ret;
}
#endif  // Z_FEATURE_QUERY == 1

#endif  // Z_FEATURE_LIVELINESS == 1
