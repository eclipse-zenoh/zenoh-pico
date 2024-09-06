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
#ifndef INCLUDE_ZENOH_PICO_API_LIVELINESS_H
#define INCLUDE_ZENOH_PICO_API_LIVELINESS_H

#include <stdbool.h>
#include <stdint.h>

#include "olv_macros.h"
#include "zenoh-pico/api/types.h"

typedef struct {
    // TODO(sashacmc): add real data
    uint8_t __dummy;
} _z_liveliness_token_t;

_Z_OWNED_TYPE_VALUE(_z_liveliness_token_t, liveliness_token)

/**
 * The options for `z_liveliness_declare_token()`.
 */
typedef struct z_liveliness_declaration_options_t {
    uint8_t __dummy;
} z_liveliness_declaration_options_t;

/**
 * The options for `z_liveliness_declare_subscriber()`
 */
typedef struct z_liveliness_subscriber_options_t {
    uint8_t __dummy;
} z_liveliness_subscriber_options_t;

/**
 * The options for `z_liveliness_get()`
 */
typedef struct z_liveliness_get_options_t {
    uint32_t timeout_ms;
} z_liveliness_get_options_t;

/**
 * Constructs default value for `z_liveliness_declaration_options_t`.
 */
int8_t z_liveliness_declaration_options_default(z_liveliness_declaration_options_t *options);

/**
 * Declares a subscriber on liveliness tokens that intersect `key_expr`.
 *
 * @param token: An uninitialized memory location where subscriber will be constructed.
 * @param session: The Zenoh session.
 * @param key_expr: The key expression to subscribe to.
 * @param callback: The callback function that will be called each time a liveliness token status is changed.
 * @param _options: The options to be passed to the liveliness subscriber declaration.
 *
 * @return 0 in case of success, negative error values otherwise.
 */
int8_t z_liveliness_declare_subscriber(z_owned_subscriber_t *token, const z_loaned_session_t *session,
                                       const z_loaned_keyexpr_t *key_expr, z_moved_closure_sample_t *callback,
                                       z_liveliness_subscriber_options_t *_options);
/**
 * Constructs and declares a liveliness token on the network.
 *
 * Liveliness token subscribers on an intersecting key expression will receive a PUT sample when connectivity
 * is achieved, and a DELETE sample if it's lost.
 *
 * @param token: An uninitialized memory location where liveliness token will be constructed.
 * @param session: A Zenos session to declare the liveliness token.
 * @param key_expr: A keyexpr to declare a liveliess token for.
 * @param _options: Liveliness token declaration properties.
 */
int8_t z_liveliness_declare_token(z_owned_liveliness_token_t *token, const z_loaned_session_t *session,
                                  const z_loaned_keyexpr_t *key_expr,
                                  const z_liveliness_declaration_options_t *_options);

/**
 * Queries liveliness tokens currently on the network with a key expression intersecting with `key_expr`.
 *
 * @param session: The Zenoh session.
 * @param key_expr: The key expression to query liveliness tokens for.
 * @param callback: The callback function that will be called for each received reply.
 * @param options: Additional options for the liveliness get operation.
 */
int8_t z_liveliness_get(const z_loaned_session_t *session, const z_loaned_keyexpr_t *key_expr,
                        z_moved_closure_reply_t *callback, z_liveliness_get_options_t *options);

/**
 * Constructs default value `z_liveliness_get_options_t`.
 */
int8_t z_liveliness_get_options_default(z_liveliness_get_options_t *options);

/**
 * Constucts default value for `z_liveliness_declare_subscriber_options_t`.
 */
int8_t z_liveliness_subscriber_options_default(z_liveliness_subscriber_options_t *options);

/**
 * Undeclares liveliness token, frees memory and resets it to a gravestone state.
 */
int8_t z_liveliness_token_drop(z_moved_liveliness_token_t *token);

/**
 * Borrows token.
 */
const z_loaned_liveliness_token_t *z_liveliness_token_loan(const z_owned_liveliness_token_t *token);

/**
 * Destroys a liveliness token, notifying subscribers of its destruction.
 */
int8_t z_liveliness_undeclare_token(z_moved_liveliness_token_t *token);

#endif  // INCLUDE_ZENOH_PICO_API_LIVELINESS_H
