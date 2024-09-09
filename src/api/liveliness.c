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
#include "zenoh-pico/utils/result.h"

int8_t z_liveliness_subscriber_options_default(z_liveliness_subscriber_options_t *options) {
    options->__dummy = 0;
    return _Z_RES_OK;
}

int8_t z_liveliness_declare_subscriber(z_owned_subscriber_t *token, const z_loaned_session_t *session,
                                       const z_loaned_keyexpr_t *key_expr, z_moved_closure_sample_t *callback,
                                       z_liveliness_subscriber_options_t *_options) {
    // TODO(sashacmc): Implement
    (void)token;
    (void)session;
    (void)key_expr;
    (void)callback;
    (void)_options;
    return _Z_RES_OK;
}

int8_t z_liveliness_declaration_options_default(z_liveliness_declaration_options_t *options) {
    options->__dummy = 0;
    return _Z_RES_OK;
}

int8_t z_liveliness_declare_token(z_owned_liveliness_token_t *token, const z_loaned_session_t *session,
                                  const z_loaned_keyexpr_t *key_expr,
                                  const z_liveliness_declaration_options_t *_options) {
    // TODO(sashacmc): Implement
    (void)token;
    (void)session;
    (void)key_expr;
    (void)_options;
    return _Z_RES_OK;
}

int8_t z_liveliness_get_options_default(z_liveliness_get_options_t *options) {
    options->timeout_ms = 0;  // TODO(sashacmc): correct value;
    return _Z_RES_OK;
}

int8_t z_liveliness_get(const z_loaned_session_t *session, const z_loaned_keyexpr_t *key_expr,
                        z_moved_closure_reply_t *callback, z_liveliness_get_options_t *options) {
    // TODO(sashacmc): Implement
    (void)session;
    (void)key_expr;
    (void)callback;
    (void)options;
    return _Z_RES_OK;
}

int8_t z_liveliness_token_drop(z_moved_liveliness_token_t *token) {
    // TODO(sashacmc): Implement
    (void)token;
    return _Z_RES_OK;
}

int8_t z_liveliness_undeclare_token(z_moved_liveliness_token_t *token) {
    // TODO(sashacmc): Implement
    (void)token;
    return _Z_RES_OK;
}
