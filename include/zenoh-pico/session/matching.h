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

#ifndef INCLUDE_ZENOH_PICO_SESSION_MATCHING_H
#define INCLUDE_ZENOH_PICO_SESSION_MATCHING_H

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/session/session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool matching;  // true if there exist matching Zenoh entities, false otherwise.
} _z_matching_status_t;

typedef void (*_z_closure_matching_status_callback_t)(const _z_matching_status_t *status, void *arg);

typedef struct {
    void *context;
    _z_closure_matching_status_callback_t call;
    _z_drop_handler_t drop;
} _z_closure_matching_status_t;

#if Z_FEATURE_MATCHING == 1

#define _Z_MATCHING_LISTENER_CTX_NULL_ID 0xFFFFFFFF

typedef struct _z_matching_listener_ctx_t {
    uint32_t decl_id;
    _z_closure_matching_status_t callback;
} _z_matching_listener_ctx_t;

typedef struct {
    uint32_t interest_id;
    _z_zint_t entity_id;
    _z_matching_listener_ctx_t *ctx;
} _z_matching_listener_state_t;

_z_matching_listener_ctx_t *_z_matching_listener_ctx_new(_z_closure_matching_status_t callback);
void _z_matching_listener_ctx_clear(_z_matching_listener_ctx_t *ctx);

_z_matching_listener_state_t *_z_matching_listener_state_new(uint32_t interest_id, _z_zint_t entity_id,
                                                             _z_matching_listener_ctx_t *ctx);
void _z_matching_listener_state_clear(_z_matching_listener_state_t *state);

_Z_ELEM_DEFINE(_z_matching_listener, _z_matching_listener_state_t, _z_noop_size, _z_matching_listener_state_clear,
               _z_noop_copy, _z_noop_move, _z_noop_eq, _z_noop_cmp, _z_noop_hash)
_Z_INT_MAP_DEFINE(_z_matching_listener, _z_matching_listener_state_t)
#endif  // Z_FEATURE_MATCHING == 1

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_SESSION_MATCHING_H */
