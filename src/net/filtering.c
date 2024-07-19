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

#include "zenoh-pico/api/types.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

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
                    ctx->state = WRITE_FILTER_OFF;
                    ctx->decl_id = msg->id;
                    break;

                default:  // Nothing to do
                    break;
            }
            break;
        // Remove filter if we receive a subscribe
        case WRITE_FILTER_ACTIVE:
            if (msg->type == _Z_INTEREST_MSG_TYPE_DECL_SUBSCRIBER) {
                ctx->state = WRITE_FILTER_OFF;
                ctx->decl_id = msg->id;
            }
            break;
        // Activate filter if subscribe is removed
        case WRITE_FILTER_OFF:
            if ((msg->type == _Z_INTEREST_MSG_TYPE_UNDECL_SUBSCRIBER) && (ctx->decl_id == msg->id)) {
                ctx->state = WRITE_FILTER_ACTIVE;
                ctx->decl_id = 0;
            }
            break;
        // Nothing to do
        default:
            break;
    }
}

int8_t _z_write_filter_create(_z_publisher_t *pub) {
    uint8_t flags = _Z_INTEREST_FLAG_KEYEXPRS | _Z_INTEREST_FLAG_SUBSCRIBERS | _Z_INTEREST_FLAG_RESTRICTED |
                    _Z_INTEREST_FLAG_CURRENT | _Z_INTEREST_FLAG_FUTURE | _Z_INTEREST_FLAG_AGGREGATE;
    _z_writer_filter_ctx_t *ctx = (_z_writer_filter_ctx_t *)z_malloc(sizeof(_z_writer_filter_ctx_t));

    if (ctx == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    ctx->state = WRITE_FILTER_INIT;
    ctx->decl_id = 0;

    pub->_filter.ctx = ctx;
    pub->_filter._interest_id = _z_add_interest(_Z_RC_IN_VAL(&pub->_zn), _z_keyexpr_alias(pub->_key),
                                                _z_write_filter_callback, flags, (void *)ctx);
    if (pub->_filter._interest_id == 0) {
        z_free(ctx);
        return _Z_ERR_GENERIC;
    }
    return _Z_RES_OK;
}

int8_t _z_write_filter_destroy(const _z_publisher_t *pub) {
    _Z_RETURN_IF_ERR(_z_remove_interest(_Z_RC_IN_VAL(&pub->_zn), pub->_filter._interest_id));
    z_free(pub->_filter.ctx);
    return _Z_RES_OK;
}

_Bool _z_write_filter_active(const _z_publisher_t *pub) { return (pub->_filter.ctx->state == WRITE_FILTER_ACTIVE); }

#else
int8_t _z_write_filter_create(_z_publisher_t *pub) {
    _ZP_UNUSED(pub);
    return _Z_RES_OK;
}

int8_t _z_write_filter_destroy(const _z_publisher_t *pub) {
    _ZP_UNUSED(pub);
    return _Z_RES_OK;
}

_Bool _z_write_filter_active(const _z_publisher_t *pub) {
    _ZP_UNUSED(pub);
    return false;
}

#endif
