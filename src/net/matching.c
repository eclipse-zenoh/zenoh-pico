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

#include "zenoh-pico/net/matching.h"

#include <stddef.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/definitions/interest.h"
#include "zenoh-pico/session/matching.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_MATCHING == 1
static void _z_matching_listener_callback(const _z_interest_msg_t *msg, void *arg) {
    _z_matching_listener_ctx_t *ctx = (_z_matching_listener_ctx_t *)arg;
    switch (msg->type) {
        case _Z_INTEREST_MSG_TYPE_DECL_SUBSCRIBER:
        case _Z_INTEREST_MSG_TYPE_DECL_QUERYABLE: {
            if (ctx->decl_id == _Z_MATCHING_LISTENER_CTX_NULL_ID) {
                ctx->decl_id = msg->id;
                z_matching_status_t status = {.matching = true};
                z_closure_matching_status_call(&ctx->callback, &status);
            }
            break;
        }

        case _Z_INTEREST_MSG_TYPE_UNDECL_SUBSCRIBER:
        case _Z_INTEREST_MSG_TYPE_UNDECL_QUERYABLE: {
            if (ctx->decl_id == msg->id) {
                ctx->decl_id = _Z_MATCHING_LISTENER_CTX_NULL_ID;
                z_matching_status_t status = {.matching = false};
                z_closure_matching_status_call(&ctx->callback, &status);
            }
            break;
        }

        default:
            break;
    }
}

_z_matching_listener_t _z_matching_listener_declare(_z_session_rc_t *zn, const _z_keyexpr_t *key, _z_zint_t entity_id,
                                                    uint8_t interest_type_flag, _z_closure_matching_status_t callback) {
    uint8_t flags = interest_type_flag | _Z_INTEREST_FLAG_RESTRICTED | _Z_INTEREST_FLAG_FUTURE |
                    _Z_INTEREST_FLAG_AGGREGATE | _Z_INTEREST_FLAG_CURRENT;
    _z_matching_listener_t ret = _z_matching_listener_null();

    _z_matching_listener_ctx_t *ctx = _z_matching_listener_ctx_new(callback);
    if (ctx == NULL) {
        return ret;
    }

    ret._interest_id = _z_add_interest(_Z_RC_IN_VAL(zn), _z_keyexpr_alias_from_user_defined(*key, true),
                                       _z_matching_listener_callback, flags, (void *)ctx);
    if (ret._interest_id == 0) {
        _z_matching_listener_ctx_clear(ctx);
        return ret;
    }

    ret._id = _z_get_entity_id(_Z_RC_IN_VAL(zn));
    ret._zn = _z_session_rc_clone_as_weak(zn);

    _z_matching_listener_intmap_insert(&_Z_RC_IN_VAL(zn)->_matching_listeners, ret._id,
                                       _z_matching_listener_state_new(ret._interest_id, entity_id, ctx));

    return ret;
}

z_result_t _z_matching_listener_entity_undeclare(_z_session_t *zn, _z_zint_t entity_id) {
    _z_matching_listener_intmap_iterator_t iter = _z_matching_listener_intmap_iterator_make(&zn->_matching_listeners);
    bool has_next = _z_matching_listener_intmap_iterator_next(&iter);
    while (has_next) {
        size_t key = _z_matching_listener_intmap_iterator_key(&iter);
        _z_matching_listener_state_t *listener = _z_matching_listener_intmap_iterator_value(&iter);
        has_next = _z_matching_listener_intmap_iterator_next(&iter);
        if (listener->entity_id == entity_id) {
            _Z_DEBUG("_z_matching_listener_entity_undeclare: entity=%i, listener=%i", (int)entity_id, (int)key);
            _z_remove_interest(zn, listener->interest_id);
            _z_matching_listener_intmap_remove(&zn->_matching_listeners, key);
        }
    }
    return _Z_RES_OK;
}

z_result_t _z_matching_listener_undeclare(_z_matching_listener_t *listener) {
    _z_session_t *zn = _Z_RC_IN_VAL(&listener->_zn);
    _z_matching_listener_intmap_remove(&zn->_matching_listeners, listener->_id);
    return _z_remove_interest(zn, listener->_interest_id);
}

void _z_matching_listener_clear(_z_matching_listener_t *listener) {
    _z_session_weak_drop(&listener->_zn);
    *listener = _z_matching_listener_null();
}

void _z_matching_listener_free(_z_matching_listener_t **listener) {
    _z_matching_listener_t *ptr = *listener;

    if (ptr != NULL) {
        _z_matching_listener_clear(ptr);

        z_free(ptr);
        *listener = NULL;
    }
}

#endif
