//
// Copyright (c) 2025 ZettaScale Technology
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

#include "zenoh-pico/api/advanced_subscriber.h"

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/primitives.h"

void ze_closure_miss_call(const ze_loaned_closure_miss_t *closure, const ze_miss_t *miss) {
    if (closure->call != NULL) {
        (closure->call)(miss, closure->context);
    }
}

#if Z_FEATURE_ADVANCED_SUBSCRIPTION == 1

_Z_OWNED_FUNCTIONS_CLOSURE_IMPL_PREFIX(ze, closure_miss, ze_closure_miss_callback_t, z_closure_drop_callback_t)

_ze_advanced_subscriber_state_t _ze_advanced_subscriber_state_null(void) {
    _ze_advanced_subscriber_state_t state = {0};
    z_internal_liveliness_token_null(&state._token);
    return state;
}

void _ze_advanced_subscriber_state_clear(_ze_advanced_subscriber_state_t *state) {
    if (state->_has_token) {
        z_liveliness_token_drop(z_liveliness_token_move(&state->_token));
    }
}

static bool _ze_advanced_subscriber_state_check(const _ze_advanced_subscriber_state_t *state) {
    return (!state->_has_token || z_internal_liveliness_token_check(&state->_token));
}

bool _ze_advanced_subscriber_check(const _ze_advanced_subscriber_t *sub) {
    return (z_internal_subscriber_check(&sub->_subscriber) &&
            (!sub->_has_liveliness_subscriber || z_internal_subscriber_check(&sub->_liveliness_subscriber)) &&
            (!sub->_has_heartbeat_subscriber || z_internal_subscriber_check(&sub->_heartbeat_subscriber)) &&
            _ze_advanced_subscriber_state_check(sub->_state._val));
}

_ze_advanced_subscriber_t _ze_advanced_subscriber_null(void) {
    _ze_advanced_subscriber_t subscriber = {0};
    z_internal_subscriber_null(&subscriber._subscriber);
    z_internal_subscriber_null(&subscriber._liveliness_subscriber);
    z_internal_subscriber_null(&subscriber._heartbeat_subscriber);
    subscriber._state = _ze_advanced_subscriber_state_simple_rc_null();

    return subscriber;
}

z_result_t _ze_advanced_subscriber_clear(_ze_advanced_subscriber_t *sub) {
    _z_subscriber_clear(&sub->_subscriber._val);
    _z_subscriber_clear(&sub->_liveliness_subscriber._val);
    _z_subscriber_clear(&sub->_heartbeat_subscriber._val);
    _ze_advanced_subscriber_state_simple_rc_drop(&sub->_state);

    *sub = _ze_advanced_subscriber_null();
    return _Z_RES_OK;
}

z_result_t _ze_advanced_subscriber_drop(_ze_advanced_subscriber_t *sub) {
    if (sub == NULL || !_ze_advanced_subscriber_check(sub)) {
        return _Z_ERR_ENTITY_UNKNOWN;
    }

    z_result_t ret = z_undeclare_subscriber(z_subscriber_move(&sub->_subscriber));
    if (sub->_has_liveliness_subscriber) {
        z_undeclare_subscriber(z_subscriber_move(&sub->_liveliness_subscriber));
    }
    if (sub->_has_heartbeat_subscriber) {
        z_undeclare_subscriber(z_subscriber_move(&sub->_heartbeat_subscriber));
    }
    _ze_advanced_subscriber_state_simple_rc_drop(&sub->_state);

    *sub = _ze_advanced_subscriber_null();
    return ret;
}

_Z_OWNED_FUNCTIONS_VALUE_NO_COPY_NO_MOVE_IMPL_PREFIX(ze, _ze_advanced_subscriber_t, advanced_subscriber,
                                                     _ze_advanced_subscriber_check, _ze_advanced_subscriber_null,
                                                     _ze_advanced_subscriber_drop)

static bool _ze_advanced_subscriber_handle_sample(_ze_advanced_subscriber_state_t *state, z_loaned_sample_t *sample) {
    if (state->_callback != NULL) {
        state->_callback(sample, state->_ctx);
    }
    return true;
}

void _ze_advanced_subscriber_subscriber_callback(z_loaned_sample_t *sample, void *ctx) {
    _ze_advanced_subscriber_state_simple_rc_t *rc_state = (_ze_advanced_subscriber_state_simple_rc_t *)ctx;
    if (!_Z_SIMPLE_RC_IS_NULL(rc_state)) {
        _ze_advanced_subscriber_handle_sample(_ze_advanced_subscriber_state_simple_rc_value(rc_state), sample);
    }
}

void _ze_advanced_subscriber_subscriber_drop_handler(void *ctx) {
    _ze_advanced_subscriber_state_simple_rc_t *rc_state = (_ze_advanced_subscriber_state_simple_rc_t *)ctx;
    if (!_Z_SIMPLE_RC_IS_NULL(rc_state)) {
        _ze_advanced_subscriber_state_t *state = rc_state->_val;
        if (state->_dropper != NULL) {
            state->_dropper(state->_ctx);
        }
        _ze_advanced_subscriber_state_simple_rc_drop(rc_state);
        z_free(ctx);
    }
}

void _ze_advanced_subscriber_reply_handler(z_loaned_reply_t *reply, void *ctx) {
    (void)(reply);
    (void)(ctx);
}

void _ze_advanced_subscriber_reply_drop_handler(void *ctx) {
    _ze_advanced_subscriber_state_simple_rc_t *rc_state = (_ze_advanced_subscriber_state_simple_rc_t *)ctx;
    if (!_Z_SIMPLE_RC_IS_NULL(rc_state)) {
        _ze_advanced_subscriber_state_simple_rc_drop(rc_state);
        z_free(ctx);
    }
}

void _ze_advanced_subscriber_liveliness_callback(z_loaned_sample_t *sample, void *ctx) {
    (void)(ctx);
    (void)(sample);
}

void _ze_advanced_subscriber_liveliness_drop_handler(void *ctx) {
    _ze_advanced_subscriber_state_simple_rc_t *rc_state = (_ze_advanced_subscriber_state_simple_rc_t *)ctx;
    if (!_Z_SIMPLE_RC_IS_NULL(rc_state)) {
        _ze_advanced_subscriber_state_simple_rc_drop(rc_state);
        z_free(ctx);
    }
}

void _ze_advanced_subscriber_heartbeat_callback(z_loaned_sample_t *sample, void *ctx) {
    (void)(ctx);
    (void)(sample);
}

void _ze_advanced_subscriber_heartbeat_drop_handler(void *ctx) {
    _ze_advanced_subscriber_state_simple_rc_t *rc_state = (_ze_advanced_subscriber_state_simple_rc_t *)ctx;
    if (!_Z_SIMPLE_RC_IS_NULL(rc_state)) {
        _ze_advanced_subscriber_state_simple_rc_drop(rc_state);
        z_free(ctx);
    }
}

// suffix = KE_ADV_PREFIX / KE_SUB / ZID / EID / [ metadata | KE_EMPTY ]
static z_result_t _ze_advanced_subscriber_ke_suffix(z_owned_keyexpr_t *suffix, const z_entity_global_id_t id,
                                                    const z_loaned_keyexpr_t *metadata) {
    z_internal_keyexpr_null(suffix);
    _Z_RETURN_IF_ERR(_Z_KEYEXPR_APPEND_STR_ARRAY(suffix, _Z_KEYEXPR_ADV_PREFIX, _Z_KEYEXPR_SUB));

    z_id_t zid = z_entity_global_id_zid(&id);
    z_owned_string_t zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(&zid, &zid_str), z_keyexpr_drop(z_keyexpr_move(suffix)));

    _Z_CLEAN_RETURN_IF_ERR(
        _z_keyexpr_append_substr(suffix, z_string_data(z_string_loan(&zid_str)), z_string_len(z_string_loan(&zid_str))),
        z_string_drop(z_string_move(&zid_str));
        z_keyexpr_drop(z_keyexpr_move(suffix)));
    z_string_drop(z_string_move(&zid_str));

    char buffer[21];
    uint32_t eid = z_entity_global_id_eid(&id);
    snprintf(buffer, sizeof(buffer), "%u", eid);
    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append_str(suffix, buffer), z_keyexpr_drop(z_keyexpr_move(suffix)));

    if (metadata != NULL) {
        _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append(suffix, metadata), z_keyexpr_drop(z_keyexpr_move(suffix)));
    } else {
        _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append_str(suffix, _Z_KEYEXPR_EMPTY), z_keyexpr_drop(z_keyexpr_move(suffix)));
    }

    return _Z_RES_OK;
}

z_result_t ze_declare_advanced_subscriber(const z_loaned_session_t *zs, ze_owned_advanced_subscriber_t *sub,
                                          const z_loaned_keyexpr_t *keyexpr, z_moved_closure_sample_t *callback,
                                          ze_advanced_subscriber_options_t *options) {
    sub->_val = _ze_advanced_subscriber_null();

    // Set options
    ze_advanced_subscriber_options_t opt;
    ze_advanced_subscriber_options_default(&opt);
    if (options != NULL) {
        opt = *options;
    }

    // Create Advanced Subscriber state
    _ze_advanced_subscriber_state_t state = _ze_advanced_subscriber_state_null();
    state._callback = callback->_this._val.call;
    state._dropper = callback->_this._val.drop;
    state._ctx = callback->_this._val.context;
    z_internal_closure_sample_null(&callback->_this);

    sub->_val._state = _ze_advanced_subscriber_state_simple_rc_new_from_val(&state);
    if (_Z_SIMPLE_RC_IS_NULL(&sub->_val._state)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    // Declare subscriber
    _ze_advanced_subscriber_state_simple_rc_t *sub_state =
        _ze_advanced_subscriber_state_simple_rc_clone_as_ptr(&sub->_val._state);
    if (_Z_SIMPLE_RC_IS_NULL(sub_state)) {
        _ze_advanced_subscriber_state_simple_rc_drop(&sub->_val._state);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    z_owned_closure_sample_t subcriber_callback;
    _Z_CLEAN_RETURN_IF_ERR(z_closure_sample(&subcriber_callback, _ze_advanced_subscriber_subscriber_callback,
                                            _ze_advanced_subscriber_subscriber_drop_handler, sub_state),
                           _ze_advanced_subscriber_state_simple_rc_drop(sub_state);
                           z_free(sub_state); _ze_advanced_subscriber_state_simple_rc_drop(&sub->_val._state));
    _Z_CLEAN_RETURN_IF_ERR(z_declare_subscriber(zs, &sub->_val._subscriber, keyexpr,
                                                z_closure_sample_move(&subcriber_callback), &opt.subscriber_options),
                           _ze_advanced_subscriber_state_simple_rc_drop(sub_state);
                           z_free(sub_state); _ze_advanced_subscriber_state_simple_rc_drop(&sub->_val._state));

    // Common keyexpr for subscribing to publisher liveliness and heartbeat and for querying history
    z_owned_keyexpr_t ke_pub;
    _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_clone(&ke_pub, keyexpr), _ze_advanced_subscriber_drop(&sub->_val));
    _Z_CLEAN_RETURN_IF_ERR(
        _Z_KEYEXPR_APPEND_STR_ARRAY(&ke_pub, _Z_KEYEXPR_ADV_PREFIX, _Z_KEYEXPR_PUB, _Z_KEYEXPR_STARSTAR),
        z_keyexpr_drop(z_keyexpr_move(&ke_pub));
        _ze_advanced_subscriber_drop(&sub->_val));

    if (opt.history.is_enabled) {
        // TODO: Query initial history

        // Declare liveliness subscriber on keyexpr / KE_ADV_PREFIX / KE_PUB / KE_STARSTAR
        if (opt.history.detect_late_publishers) {
            z_liveliness_subscriber_options_t liveliness_options;
            z_liveliness_subscriber_options_default(&liveliness_options);
            liveliness_options.history = true;

            _ze_advanced_subscriber_state_simple_rc_t *liveliness_sub_state =
                _ze_advanced_subscriber_state_simple_rc_clone_as_ptr(&sub->_val._state);
            if (_Z_SIMPLE_RC_IS_NULL(sub_state)) {
                z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                _ze_advanced_subscriber_drop(&sub->_val);
                return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            }

            z_owned_closure_sample_t liveliness_callback;
            _Z_CLEAN_RETURN_IF_ERR(
                z_closure_sample(&liveliness_callback, _ze_advanced_subscriber_liveliness_callback,
                                 _ze_advanced_subscriber_liveliness_drop_handler, liveliness_sub_state),
                _ze_advanced_subscriber_state_simple_rc_drop(liveliness_sub_state);
                z_free(liveliness_sub_state); z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                _ze_advanced_subscriber_drop(&sub->_val));
            _Z_CLEAN_RETURN_IF_ERR(
                z_liveliness_declare_subscriber(zs, &sub->_val._liveliness_subscriber, z_keyexpr_loan(&ke_pub),
                                                z_closure_sample_move(&liveliness_callback), &liveliness_options),
                _ze_advanced_subscriber_state_simple_rc_drop(liveliness_sub_state);
                z_free(liveliness_sub_state); z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                _ze_advanced_subscriber_drop(&sub->_val));
            sub->_val._has_liveliness_subscriber = true;
        }
    }

    // Heartbeat subscriber
    if (opt.recovery.is_enabled && opt.recovery.last_sample_miss_detection.is_enabled &&
        opt.recovery.last_sample_miss_detection.periodic_queries_period_ms == 0) {
        _ze_advanced_subscriber_state_simple_rc_t *heartbeat_sub_state =
            _ze_advanced_subscriber_state_simple_rc_clone_as_ptr(&sub->_val._state);
        if (_Z_SIMPLE_RC_IS_NULL(heartbeat_sub_state)) {
            z_keyexpr_drop(z_keyexpr_move(&ke_pub));
            _ze_advanced_subscriber_drop(&sub->_val);
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }

        z_owned_closure_sample_t heartbeat_callback;
        _Z_CLEAN_RETURN_IF_ERR(z_closure_sample(&heartbeat_callback, _ze_advanced_subscriber_heartbeat_callback,
                                                _ze_advanced_subscriber_heartbeat_drop_handler, heartbeat_sub_state),
                               _ze_advanced_subscriber_state_simple_rc_drop(heartbeat_sub_state);
                               z_free(heartbeat_sub_state); z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                               _ze_advanced_subscriber_drop(&sub->_val));
        _Z_CLEAN_RETURN_IF_ERR(z_declare_subscriber(zs, &sub->_val._heartbeat_subscriber, z_keyexpr_loan(&ke_pub),
                                                    z_closure_sample_move(&heartbeat_callback), NULL),
                               _ze_advanced_subscriber_state_simple_rc_drop(heartbeat_sub_state);
                               z_free(heartbeat_sub_state); z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                               _ze_advanced_subscriber_drop(&sub->_val));
        sub->_val._has_heartbeat_subscriber = true;
    }

    // Declare liveliness token on keyexpr/suffix
    if (opt.subscriber_detection) {
        z_entity_global_id_t id = z_subscriber_id(z_subscriber_loan(&sub->_val._subscriber));
        z_owned_keyexpr_t suffix;
        _Z_CLEAN_RETURN_IF_ERR(_ze_advanced_subscriber_ke_suffix(&suffix, id, opt.subscriber_detection_metadata),
                               z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                               _ze_advanced_subscriber_drop(&sub->_val));
        z_owned_keyexpr_t ke;
        _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_join(&ke, keyexpr, z_keyexpr_loan(&suffix)),
                               z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                               z_keyexpr_drop(z_keyexpr_move(&suffix)); _ze_advanced_subscriber_drop(&sub->_val));
        _Z_CLEAN_RETURN_IF_ERR(z_liveliness_declare_token(zs, &state._token, z_keyexpr_loan(&ke), NULL),
                               z_keyexpr_drop(z_keyexpr_move(&ke_pub));
                               z_keyexpr_drop(z_keyexpr_move(&suffix)); z_keyexpr_drop(z_keyexpr_move(&ke));
                               _ze_advanced_subscriber_drop(&sub->_val));
        state._has_token = true;
        z_keyexpr_drop(z_keyexpr_move(&ke));
        z_keyexpr_drop(z_keyexpr_move(&suffix));
    }
    z_keyexpr_drop(z_keyexpr_move(&ke_pub));

    return _Z_RES_OK;
}

z_result_t ze_declare_background_advanced_subscriber(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                                                     z_moved_closure_sample_t *callback,
                                                     ze_advanced_subscriber_options_t *options) {
    ze_owned_advanced_subscriber_t sub;
    _Z_RETURN_IF_ERR(ze_declare_advanced_subscriber(zs, &sub, keyexpr, callback, options));
    _ze_advanced_subscriber_clear(&sub._val);
    return _Z_RES_OK;
}

z_result_t ze_undeclare_advanced_subscriber(ze_moved_advanced_subscriber_t *sub) {
    return _ze_advanced_subscriber_drop(&sub->_this._val);
}

const z_loaned_keyexpr_t *ze_advanced_subscriber_keyexpr(const ze_loaned_advanced_subscriber_t *sub) {
    return z_subscriber_keyexpr(z_subscriber_loan(&sub->_subscriber));
}

z_entity_global_id_t ze_advanced_subscriber_id(const ze_loaned_advanced_subscriber_t *sub) {
    return z_subscriber_id(z_subscriber_loan(&sub->_subscriber));
}

// TODO
z_result_t ze_advanced_subscriber_declare_sample_miss_listener(const ze_loaned_advanced_subscriber_t *subscriber,
                                                               ze_owned_sample_miss_listener_t *sample_miss_listener,
                                                               ze_moved_closure_miss_t *callback) {
    (void)(subscriber);
    (void)(sample_miss_listener);
    (void)(callback);
    return _Z_ERR_GENERIC;
}

// TODO
z_result_t ze_advanced_subscriber_declare_background_sample_miss_listener(
    const ze_loaned_advanced_subscriber_t *subscriber, ze_moved_closure_miss_t *callback) {
    (void)(subscriber);
    (void)(callback);
    return _Z_ERR_GENERIC;
}

// TODO
z_result_t ze_advanced_subscriber_detect_publishers(const ze_loaned_advanced_subscriber_t *subscriber,
                                                    z_owned_subscriber_t *liveliness_subscriber,
                                                    z_moved_closure_sample_t *callback,
                                                    z_liveliness_subscriber_options_t *options) {
    (void)(subscriber);
    (void)(liveliness_subscriber);
    (void)(callback);
    (void)(options);
    return _Z_ERR_GENERIC;
}

// TODO
z_result_t ze_advanced_subscriber_detect_publishers_background(const ze_loaned_advanced_subscriber_t *subscriber,
                                                               z_moved_closure_sample_t *callback,
                                                               z_liveliness_subscriber_options_t *options) {
    (void)(subscriber);
    (void)(callback);
    (void)(options);
    return _Z_ERR_GENERIC;
}

void ze_advanced_subscriber_history_options_default(ze_advanced_subscriber_history_options_t *options) {
    options->is_enabled = true;
    options->detect_late_publishers = false;
    options->max_samples = 0;
    options->max_age_ms = 0;
}

void ze_advanced_subscriber_last_sample_miss_detection_options_default(
    ze_advanced_subscriber_last_sample_miss_detection_options_t *options) {
    options->is_enabled = true;
    options->periodic_queries_period_ms = 0;
}

void ze_advanced_subscriber_recovery_options_default(ze_advanced_subscriber_recovery_options_t *options) {
    options->is_enabled = true;
    ze_advanced_subscriber_last_sample_miss_detection_options_default(&options->last_sample_miss_detection);
    options->last_sample_miss_detection.is_enabled = false;
}

void ze_advanced_subscriber_options_default(ze_advanced_subscriber_options_t *options) {
    z_subscriber_options_default(&options->subscriber_options);

    ze_advanced_subscriber_history_options_default(&options->history);
    options->history.is_enabled = false;

    ze_advanced_subscriber_recovery_options_default(&options->recovery);
    options->history.is_enabled = false;

    options->query_timeout_ms = 0;
    options->subscriber_detection = false;
    options->subscriber_detection_metadata = NULL;
}

#endif
