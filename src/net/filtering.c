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

#include "zenoh-pico/net/filtering.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/session/session.h"

#if Z_FEATURE_INTEREST == 1

void _z_filter_target_elem_free(void **e) {
    _z_filter_target_t *ptr = (_z_filter_target_t *)*e;
    if (ptr != NULL) {
        z_free(ptr);
        *e = NULL;
    }
}

static bool _z_filter_target_peer_eq(const void *left, const void *right) {
    const _z_filter_target_t *left_val = (const _z_filter_target_t *)left;
    const _z_filter_target_t *right_val = (const _z_filter_target_t *)right;
    return left_val->peer == right_val->peer;
}

static bool _z_filter_target_eq(const void *left, const void *right) {
    const _z_filter_target_t *left_val = (const _z_filter_target_t *)left;
    const _z_filter_target_t *right_val = (const _z_filter_target_t *)right;
    return (left_val->peer == right_val->peer) && (left_val->decl_id == right_val->decl_id);
}

#if Z_FEATURE_MULTI_THREAD == 1
static void _z_write_filter_mutex_lock(_z_writer_filter_ctx_t *ctx) { _z_mutex_lock(&ctx->mutex); }
static void _z_write_filter_mutex_unlock(_z_writer_filter_ctx_t *ctx) { _z_mutex_unlock(&ctx->mutex); }
#else
static void _z_write_filter_mutex_lock(_z_writer_filter_ctx_t *ctx) { _ZP_UNUSED(ctx); }
static void _z_write_filter_mutex_unlock(_z_writer_filter_ctx_t *ctx) { _ZP_UNUSED(ctx); }
#endif

static bool _z_write_filter_push_target(_z_writer_filter_ctx_t *ctx, _z_transport_peer_common_t *peer, uint32_t id) {
    _z_filter_target_t *target = (_z_filter_target_t *)z_malloc(sizeof(_z_filter_target_t));
    if (target == NULL) {
        _Z_ERROR("Failed to allocate filter target.");
        return false;
    }
    target->peer = (uintptr_t)peer;
    target->decl_id = id;
    _z_write_filter_mutex_lock(ctx);
    ctx->targets = _z_filter_target_list_push(ctx->targets, target);
    _z_write_filter_mutex_unlock(ctx);
    if (ctx->targets == NULL) {  // Allocation can fail
        return false;
    }
    return true;
}

static void _z_write_filter_callback(const _z_interest_msg_t *msg, _z_transport_peer_common_t *peer, void *arg) {
    _z_writer_filter_ctx_t *ctx = (_z_writer_filter_ctx_t *)arg;
    // Process message
    switch (msg->type) {
        case _Z_INTEREST_MSG_TYPE_DECL_SUBSCRIBER:
        case _Z_INTEREST_MSG_TYPE_DECL_QUERYABLE:
            _z_write_filter_push_target(ctx, peer, msg->id);
            break;
        case _Z_INTEREST_MSG_TYPE_UNDECL_SUBSCRIBER:
        case _Z_INTEREST_MSG_TYPE_UNDECL_QUERYABLE: {
            _z_filter_target_t target = {.decl_id = msg->id, .peer = (uintptr_t)peer};
            _z_write_filter_mutex_lock(ctx);
            ctx->targets = _z_filter_target_list_drop_filter(ctx->targets, _z_filter_target_eq, &target);
            _z_write_filter_mutex_unlock(ctx);
        } break;
        case _Z_INTEREST_MSG_TYPE_CONNECTION_DROPPED: {
            _z_filter_target_t target = {.decl_id = 0, .peer = (uintptr_t)peer};
            _z_write_filter_mutex_lock(ctx);
            ctx->targets = _z_filter_target_list_drop_filter(ctx->targets, _z_filter_target_peer_eq, &target);
            _z_write_filter_mutex_unlock(ctx);
        } break;
        default:
            break;
    }
    // Process filter state
    switch (ctx->state) {
        default: // Incorrect values are treated as init
        case WRITE_FILTER_INIT:
            // Update init state
            if (ctx->targets == NULL) {
                ctx->state = WRITE_FILTER_ACTIVE;
            } else {
                ctx->state = WRITE_FILTER_OFF;
            }
            break;
        case WRITE_FILTER_ACTIVE:
            // Deactivate filter if no more targets
            if (ctx->targets != NULL) {
                ctx->state = WRITE_FILTER_OFF;
            }
            break;
        case WRITE_FILTER_OFF:
            // Activate filter if no more targets
            if (ctx->targets == NULL) {
                ctx->state = WRITE_FILTER_ACTIVE;
            }
            break;
    }
}

z_result_t _z_write_filter_create(_z_session_t *zn, _z_write_filter_t *filter, _z_keyexpr_t keyexpr,
                                  uint8_t interest_flag) {
    uint8_t flags = interest_flag | _Z_INTEREST_FLAG_KEYEXPRS | _Z_INTEREST_FLAG_RESTRICTED | _Z_INTEREST_FLAG_CURRENT |
                    _Z_INTEREST_FLAG_FUTURE | _Z_INTEREST_FLAG_AGGREGATE;
    _z_writer_filter_ctx_t *ctx = (_z_writer_filter_ctx_t *)z_malloc(sizeof(_z_writer_filter_ctx_t));

    if (ctx == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_init(&ctx->mutex));
#endif
    // FIXME: Require interest protocol profile implementation
    // ctx->state = (zn->_mode == Z_WHATAMI_CLIENT) ? WRITE_FILTER_INIT : WRITE_FILTER_ACTIVE;
    ctx->state = WRITE_FILTER_INIT;
    ctx->targets = _z_filter_target_list_new();

    filter->ctx = ctx;
    filter->_interest_id = _z_add_interest(zn, keyexpr, _z_write_filter_callback, flags, (void *)ctx);
    if (filter->_interest_id == 0) {
        z_free(ctx);
        return _Z_ERR_GENERIC;
    }
    return _Z_RES_OK;
}

z_result_t _z_write_filter_destroy(_z_session_t *zn, _z_write_filter_t *filter) {
    if (filter->ctx != NULL) {
        z_result_t res = _z_remove_interest(zn, filter->_interest_id);
        _z_filter_target_list_free(&filter->ctx->targets);
#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_drop(&filter->ctx->mutex);
#endif
        z_free(filter->ctx);
        filter->ctx = NULL;
        return res;
    }
    return _Z_RES_OK;
}

bool _z_write_filter_active(const _z_write_filter_t *filter) {
    return filter->ctx != NULL && filter->ctx->state == WRITE_FILTER_ACTIVE;
}

#else
z_result_t _z_write_filter_create(_z_session_t *zn, _z_write_filter_t *filter, _z_keyexpr_t keyexpr,
                                  uint8_t interest_flag) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(keyexpr);
    _ZP_UNUSED(filter);
    _ZP_UNUSED(interest_flag);
    return _Z_RES_OK;
}

z_result_t _z_write_filter_destroy(_z_session_t *zn, _z_write_filter_t *filter) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(filter);
    return _Z_RES_OK;
}

bool _z_write_filter_active(const _z_write_filter_t *filter) {
    _ZP_UNUSED(filter);
    return false;
}

#endif
