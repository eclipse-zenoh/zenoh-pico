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
//

#include "zenoh-pico/api/liveliness.h"

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/net/liveliness.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_LIVELINESS == 1

/**************** Liveliness Token ****************/

_Bool _z_liveliness_token_check(const _z_liveliness_token_t *token) {
    _z_keyexpr_check(&token->_key);
    return true;
}

_z_liveliness_token_t _z_liveliness_token_null(void) {
    _z_liveliness_token_t s = {0};
    s._key = _z_keyexpr_null();
    return s;
}

z_result_t _z_liveliness_token_clear(_z_liveliness_token_t *token) {
    z_result_t ret = _Z_RES_OK;
    if (_Z_RC_IS_NULL(&token->_zn)) {
        return ret;
    }
    _z_session_rc_t sess_rc = _z_session_weak_upgrade_if_open(&token->_zn);
    if (!_Z_RC_IS_NULL(&sess_rc)) {
        ret = _z_undeclare_liveliness_token(token);
        _z_session_rc_drop(&sess_rc);
    }
    _z_session_weak_drop(&token->_zn);
    _z_keyexpr_clear(&token->_key);
    *token = _z_liveliness_token_null();

    return ret;
}

_Z_OWNED_FUNCTIONS_VALUE_NO_COPY_NO_MOVE_IMPL(_z_liveliness_token_t, liveliness_token, _z_liveliness_token_check,
                                              _z_liveliness_token_null, _z_liveliness_token_clear)

z_result_t z_liveliness_token_options_default(z_liveliness_token_options_t *options) {
    options->__dummy = 0;
    return _Z_RES_OK;
}

z_result_t z_liveliness_declare_token(const z_loaned_session_t *zs, z_owned_liveliness_token_t *token,
                                      const z_loaned_keyexpr_t *keyexpr, const z_liveliness_token_options_t *options) {
    (void)options;

    _z_keyexpr_t key = _z_update_keyexpr_to_declared(_Z_RC_IN_VAL(zs), *keyexpr);

    return _z_declare_liveliness_token(zs, &token->_val, &key);
}

z_result_t z_liveliness_undeclare_token(z_moved_liveliness_token_t *token) {
    return _z_liveliness_token_clear(&token->_this._val);
}

/**************** Liveliness Subscriber ****************/

#if Z_FEATURE_SUBSCRIPTION == 1
z_result_t z_liveliness_subscriber_options_default(z_liveliness_subscriber_options_t *options) {
    options->history = false;
    return _Z_RES_OK;
}

z_result_t z_liveliness_declare_subscriber(const z_loaned_session_t *zs, z_owned_subscriber_t *sub,
                                           const z_loaned_keyexpr_t *keyexpr, z_moved_closure_sample_t *callback,
                                           z_liveliness_subscriber_options_t *options) {
    void *ctx = callback->_this._val.context;
    callback->_this._val.context = NULL;

    z_liveliness_subscriber_options_t opt;
    if (options == NULL) {
        z_liveliness_subscriber_options_default(&opt);
    } else {
        opt = *options;
    }

    _z_keyexpr_t key = _z_update_keyexpr_to_declared(_Z_RC_IN_VAL(zs), *keyexpr);

    _z_subscriber_t int_sub = _z_declare_liveliness_subscriber(zs, &key, callback->_this._val.call,
                                                               callback->_this._val.drop, opt.history, ctx);

    z_internal_closure_sample_null(&callback->_this);
    sub->_val = int_sub;

    if (!_z_subscriber_check(&sub->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    if (opt.history) {
        z_result_t ret = _z_liveliness_subscription_trigger_history(_Z_RC_IN_VAL(zs), keyexpr);
        if (ret != _Z_RES_OK) {
            return ret;
        }
    }

    return _Z_RES_OK;
}

z_result_t z_liveliness_declare_background_subscriber(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                                                      z_moved_closure_sample_t *callback,
                                                      z_liveliness_subscriber_options_t *options) {
    z_owned_subscriber_t sub;
    _Z_RETURN_IF_ERR(z_liveliness_declare_subscriber(zs, &sub, keyexpr, callback, options));
    _z_subscriber_clear(&sub._val);
    return _Z_RES_OK;
}
#endif  // Z_FEATURE_SUBSCRIPTION == 1

/**************** Liveliness Query ****************/

#if Z_FEATURE_QUERY == 1
z_result_t z_liveliness_get_options_default(z_liveliness_get_options_t *options) {
    options->timeout_ms = Z_GET_TIMEOUT_DEFAULT;
    return _Z_RES_OK;
}

z_result_t z_liveliness_get(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                            z_moved_closure_reply_t *callback, z_liveliness_get_options_t *options) {
    z_result_t ret = _Z_RES_OK;

    void *ctx = callback->_this._val.context;
    callback->_this._val.context = NULL;

    z_liveliness_get_options_t opt;
    if (options == NULL) {
        z_liveliness_get_options_default(&opt);
    } else {
        opt = *options;
    }

    _z_keyexpr_t ke = _z_keyexpr_alias_from_user_defined(*keyexpr, true);
    ret = _z_liveliness_query(_Z_RC_IN_VAL(zs), &ke, callback->_this._val.call, callback->_this._val.drop, ctx,
                              opt.timeout_ms);

    z_internal_closure_reply_null(
        &callback->_this);  // call and drop passed to _z_liveliness_query, so we nullify the closure here
    return ret;
}
#endif  // Z_FEATURE_QUERY == 1

#endif  // Z_FEATURE_LIVELINESS == 1
