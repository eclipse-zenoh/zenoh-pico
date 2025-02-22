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
static void _z_write_filter_callback(const _z_interest_msg_t *msg, void *arg) {
    _z_writer_filter_ctx_t *ctx = (_z_writer_filter_ctx_t *)arg;

    switch (ctx->state) {
        // Update init state
        case WRITE_FILTER_INIT:
            switch (msg->type) {
                case _Z_INTEREST_MSG_TYPE_FINAL:
                    ctx->state = WRITE_FILTER_ACTIVE;  // No subscribers
                    break;

                case _Z_INTEREST_MSG_TYPE_DECL_SUBSCRIBER:
                case _Z_INTEREST_MSG_TYPE_DECL_QUERYABLE:
                    ctx->state = WRITE_FILTER_OFF;
                    ctx->decl_id = msg->id;
                    break;

                default:  // Nothing to do
                    break;
            }
            break;
        // Remove filter if we receive a subscribe
        case WRITE_FILTER_ACTIVE:
            if (msg->type == _Z_INTEREST_MSG_TYPE_DECL_SUBSCRIBER || msg->type == _Z_INTEREST_MSG_TYPE_DECL_QUERYABLE) {
                ctx->state = WRITE_FILTER_OFF;
                ctx->decl_id = msg->id;
            }
            break;
        // Activate filter if subscribe is removed
        case WRITE_FILTER_OFF:
            if ((msg->type == _Z_INTEREST_MSG_TYPE_UNDECL_SUBSCRIBER ||
                 msg->type == _Z_INTEREST_MSG_TYPE_UNDECL_QUERYABLE) &&
                (ctx->decl_id == msg->id)) {
                ctx->state = WRITE_FILTER_ACTIVE;
                ctx->decl_id = 0;
            }
            break;
        // Nothing to do
        default:
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
    ctx->state = WRITE_FILTER_INIT;
    ctx->decl_id = 0;

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
