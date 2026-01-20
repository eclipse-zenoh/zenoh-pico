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

#include "zenoh-pico/net/liveliness.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/interest.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/session/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/locality.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_LIVELINESS == 1

/**************** Liveliness Token ****************/

z_result_t _z_declare_liveliness_token(const _z_session_rc_t *zn, _z_liveliness_token_t *ret_token,
                                       const _z_keyexpr_t *keyexpr) {
    *ret_token = _z_liveliness_token_null();
    uint32_t id = _z_get_entity_id(_Z_RC_IN_VAL(zn));

    _z_keyexpr_t ke;
    _Z_RETURN_IF_ERR(_z_keyexpr_declare(zn, &ke, keyexpr));
    _Z_CLEAN_RETURN_IF_ERR(_z_liveliness_register_token(_Z_RC_IN_VAL(zn), id, &ke), _z_keyexpr_clear(&ke));

    _z_wireexpr_t wireexpr = _z_keyexpr_alias_to_wire(&ke, _Z_RC_IN_VAL(zn));
    _z_declaration_t declaration = _z_make_decl_token(&wireexpr, id);
    _z_network_message_t n_msg;
    _z_n_msg_make_declare(&n_msg, declaration, _z_optional_id_make_none());
    z_result_t ret = _z_send_declare(_Z_RC_IN_VAL(zn), &n_msg);
    _z_n_msg_clear(&n_msg);

    if (ret != _Z_RES_OK) {
        _z_liveliness_unregister_token(_Z_RC_IN_VAL(zn), id);
        _z_keyexpr_clear(&ke);
        return ret;
    }

    ret_token->_id = id;
    ret_token->_zn = _z_session_rc_clone_as_weak(zn);
    ret_token->_key = ke;

    return _Z_RES_OK;
}

z_result_t _z_undeclare_liveliness_token(_z_liveliness_token_t *token) {
    if (token == NULL || _Z_RC_IS_NULL(&token->_zn)) {
        _Z_ERROR_RETURN(_Z_ERR_ENTITY_UNKNOWN);
    }
#if Z_FEATURE_SESSION_CHECK == 1
    _z_session_rc_t sess_rc = _z_session_weak_upgrade_if_open(&token->_zn);
    if (_Z_RC_IS_NULL(&sess_rc)) {
        return _Z_ERR_SESSION_CLOSED;
    }
    _z_session_t *zn = _Z_RC_IN_VAL(&sess_rc);
#else
    _z_session_t *zn = _z_session_weak_as_unsafe_ptr(&token->_zn);
#endif

    z_result_t ret;

    _z_liveliness_unregister_token(zn, token->_id);
    _z_wireexpr_t expr = _z_keyexpr_alias_to_wire(&token->_key, zn);
    _z_declaration_t declaration = _z_make_undecl_token(token->_id, &expr);
    _z_network_message_t n_msg;
    _z_n_msg_make_declare(&n_msg, declaration, _z_optional_id_make_none());
    ret = _z_send_undeclare(zn, &n_msg);
    _z_n_msg_clear(&n_msg);

#if Z_FEATURE_SESSION_CHECK == 1
    _z_session_rc_drop(&sess_rc);
#endif
    return ret;
}

/**************** Liveliness Subscriber ****************/

z_result_t _z_liveliness_subscription_trigger_history(_z_session_t *zn, const _z_subscription_t *sub) {
    z_result_t ret = _Z_RES_OK;

    _Z_DEBUG("Retrieve liveliness history for %.*s", (int)_z_string_len(&sub->_key._keyexpr),
             _z_string_data(&sub->_key._keyexpr));

    _z_keyexpr_slist_t *tokens_list = _z_keyexpr_slist_new();
    _z_session_mutex_lock(zn);
    // TODO: could we call callbacks inside the mutex? - this would allow to avoid extra keyexpr copies, list
    // allocations, in addition it would also allow to stay consistent with respect to eventual remote undeclarations,
    // i.e. if they arrive during callback execution - they will only delivered after initial history calls,
    // thus preventing potential declare/undeclare order inversion.
    // TODO: add support for local tokens
    _z_keyexpr_intmap_iterator_t iter = _z_keyexpr_intmap_iterator_make(&zn->_remote_tokens);
    while (_z_keyexpr_intmap_iterator_next(&iter)) {
        if (_z_keyexpr_intersects(&sub->_key, _z_keyexpr_intmap_iterator_value(&iter))) {
            tokens_list = _z_keyexpr_slist_push(tokens_list, _z_keyexpr_intmap_iterator_value(&iter));
        }
    }
    _z_session_mutex_unlock(zn);

    _z_keyexpr_slist_t *pos = tokens_list;
    while (pos != NULL) {
        _z_sample_t s = _z_sample_null();
        s.kind = Z_SAMPLE_KIND_PUT;
        s.keyexpr = _z_keyexpr_alias(_z_keyexpr_slist_value(pos));
        sub->_callback(&s, sub->_arg);
        _z_sample_clear(&s);
        pos = _z_keyexpr_slist_next(pos);
    }
    _z_keyexpr_slist_free(&tokens_list);

    return ret;
}

#if Z_FEATURE_SUBSCRIPTION == 1
z_result_t _z_declare_liveliness_subscriber(_z_subscriber_t *subscriber, const _z_session_rc_t *zn,
                                            const _z_keyexpr_t *keyexpr, _z_closure_sample_callback_t callback,
                                            _z_drop_handler_t dropper, bool history, void *arg) {
    *subscriber = _z_subscriber_null();
    _z_subscription_t s;
    s._id = _z_get_entity_id(_Z_RC_IN_VAL(zn));
    s._callback = callback;
    s._dropper = dropper;
    s._arg = arg;
    s._allowed_origin = z_locality_default();
    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_declare(zn, &s._key, keyexpr), _z_subscription_clear(&s));

    // Register subscription, stored at session-level, do not drop it by the end of this function.
    _z_subscription_rc_t sp_s =
        _z_register_subscription(_Z_RC_IN_VAL(zn), _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, &s);
    if (_Z_RC_IS_NULL(&sp_s)) {
        _z_subscription_clear(&s);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Build the declare message to send on the wire
    uint8_t mode = history ? (_Z_INTEREST_FLAG_CURRENT | _Z_INTEREST_FLAG_FUTURE) : _Z_INTEREST_FLAG_FUTURE;
    _z_wireexpr_t wireexpr = _z_keyexpr_alias_to_wire(&_Z_RC_IN_VAL(&sp_s)->_key, _Z_RC_IN_VAL(zn));
    _z_interest_t interest = _z_make_interest(
        &wireexpr, s._id, _Z_INTEREST_FLAG_KEYEXPRS | _Z_INTEREST_FLAG_TOKENS | _Z_INTEREST_FLAG_RESTRICTED | mode);

    _z_network_message_t n_msg;
    _z_n_msg_make_interest(&n_msg, interest);
    if (_z_send_declare(_Z_RC_IN_VAL(zn), &n_msg) != _Z_RES_OK) {
        _z_unregister_subscription(_Z_RC_IN_VAL(zn), _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, &sp_s);
        return _Z_ERR_TRANSPORT_TX_FAILED;
    }
    _z_n_msg_clear(&n_msg);
    subscriber->_entity_id = s._id;
    subscriber->_zn = _z_session_rc_clone_as_weak(zn);

    z_result_t res = _Z_RES_OK;
    if (history) {
        res = _z_liveliness_subscription_trigger_history(_Z_RC_IN_VAL(zn), _Z_RC_IN_VAL(&sp_s));
    }
    _z_subscription_rc_drop(&sp_s);
    if (res != _Z_RES_OK) {
        _z_undeclare_liveliness_subscriber(subscriber);
    }
    return _Z_RES_OK;
}

z_result_t _z_undeclare_liveliness_subscriber(_z_subscriber_t *sub) {
    if (sub == NULL || _Z_RC_IS_NULL(&sub->_zn)) {
        _Z_ERROR_RETURN(_Z_ERR_ENTITY_UNKNOWN);
    }

    _z_subscription_rc_t s =
        _z_get_subscription_by_id(_Z_RC_IN_VAL(&sub->_zn), _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, sub->_entity_id);
    if (_Z_RC_IS_NULL(&s)) {
        _Z_ERROR_RETURN(_Z_ERR_ENTITY_UNKNOWN);
    }

    _z_interest_t interest = _z_make_interest_final(_Z_RC_IN_VAL(&s)->_id);
    _z_network_message_t n_msg;
    _z_n_msg_make_interest(&n_msg, interest);
    if (_z_send_undeclare(_Z_RC_IN_VAL(&sub->_zn), &n_msg) != _Z_RES_OK) {
        _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_TX_FAILED);
    }
    _z_n_msg_clear(&n_msg);

    _z_unregister_subscription(_Z_RC_IN_VAL(&sub->_zn), _Z_SUBSCRIBER_KIND_LIVELINESS_SUBSCRIBER, &s);
    return _Z_RES_OK;
}
#endif  // Z_FEATURE_SUBSCRIPTION == 1

/**************** Liveliness Query ****************/

#if Z_FEATURE_QUERY == 1
z_result_t _z_liveliness_query(_z_session_t *zn, const _z_keyexpr_t *keyexpr, _z_closure_reply_callback_t callback,
                               _z_drop_handler_t dropper, void *arg, uint64_t timeout_ms, _z_zint_t *out_id) {
    z_result_t ret = _Z_RES_OK;

    _z_liveliness_pending_query_t pq;
    uint32_t id = _z_liveliness_get_query_id(zn);
    pq._callback = callback;
    pq._dropper = dropper;
    pq._arg = arg;
    *out_id = id;
    ret = _z_keyexpr_copy(&pq._key, keyexpr);
    _Z_SET_IF_OK(ret, _z_liveliness_register_pending_query(zn, id, &pq));
    if (ret == _Z_RES_OK) {
        _ZP_UNUSED(timeout_ms);  // Current interest in pico don't support timeout
        _z_wireexpr_t wireexpr = _z_keyexpr_alias_to_wire(keyexpr, zn);
        _z_interest_t interest = _z_make_interest(&wireexpr, id,
                                                  _Z_INTEREST_FLAG_KEYEXPRS | _Z_INTEREST_FLAG_TOKENS |
                                                      _Z_INTEREST_FLAG_RESTRICTED | _Z_INTEREST_FLAG_CURRENT);
        _z_network_message_t n_msg;
        _z_n_msg_make_interest(&n_msg, interest);
        if (_z_send_declare(zn, &n_msg) != _Z_RES_OK) {
            _z_liveliness_unregister_pending_query(zn, id);
            _Z_ERROR_LOG(_Z_ERR_TRANSPORT_TX_FAILED);
            ret = _Z_ERR_TRANSPORT_TX_FAILED;
        }
        _z_n_msg_clear(&n_msg);
    } else {
        _z_liveliness_pending_query_clear(&pq);
    }

    return ret;
}
#endif  // Z_FEATURE_QUERY == 1

#endif  // Z_FEATURE_LIVELINESS == 1
