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

#include "zenoh-pico/session/matching.h"

#if Z_FEATURE_MATCHING == 1
_z_matching_listener_ctx_t *_z_matching_listener_ctx_new(_z_closure_matching_status_t callback) {
    _z_matching_listener_ctx_t *ctx = z_malloc(sizeof(_z_matching_listener_ctx_t));

    ctx->decl_id = _Z_MATCHING_LISTENER_CTX_NULL_ID;
    ctx->callback = callback;

    return ctx;
}

void _z_matching_listener_ctx_clear(_z_matching_listener_ctx_t *ctx) {
    if (ctx->callback.drop != NULL) {
        ctx->callback.drop(ctx->callback.context);
    }
}

_z_matching_listener_state_t *_z_matching_listener_state_new(uint32_t interest_id, _z_zint_t entity_id,
                                                             _z_matching_listener_ctx_t *ctx) {
    _z_matching_listener_state_t *state = z_malloc(sizeof(_z_matching_listener_state_t));

    state->interest_id = interest_id;
    state->entity_id = entity_id;
    state->ctx = ctx;

    return state;
}

void _z_matching_listener_state_clear(_z_matching_listener_state_t *state) {
    if (state->ctx != NULL) {
        _z_matching_listener_ctx_clear(state->ctx);
        z_free(state->ctx);
    }
}
#endif  // Z_FEATURE_MATCHING == 1
