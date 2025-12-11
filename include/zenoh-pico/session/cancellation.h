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

#ifndef ZENOH_PICO_SESSION_CANCELLATION_H
#define ZENOH_PICO_SESSION_CANCELLATION_H

#include "zenoh-pico/collections/sync_group.h"
#include "zenoh-pico/collections/vec.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/system/platform.h"

typedef z_result_t (*_z_cancellation_token_on_cancel_handler_callback)(void* arg);
typedef void (*_z_cancellation_token_on_cancel_handler_dropper)(void* arg);

typedef struct {
    _z_sync_group_t _sync_group;
    _z_cancellation_token_on_cancel_handler_callback _on_cancel;
    _z_cancellation_token_on_cancel_handler_dropper _on_drop;
    void* _arg;
} _z_cancellation_token_on_cancel_handler_t;

z_result_t _z_cancellation_token_on_cancel_handler_cancel(_z_cancellation_token_on_cancel_handler_t* handler);
z_result_t _z_cancellation_token_on_cancel_handler_cancel_deadline(_z_cancellation_token_on_cancel_handler_t* handler,
                                                                   const z_clock_t* deadline);

static inline _z_cancellation_token_on_cancel_handler_t _z_cancellation_token_on_cancel_handler_null(void) {
    _z_cancellation_token_on_cancel_handler_t h = {0};
    return h;
}

static inline void _z_cancellation_token_on_cancel_handler_drop(_z_cancellation_token_on_cancel_handler_t* h) {
    if (h->_on_drop != NULL) {
        h->_on_drop(h->_arg);
    }
    _z_sync_group_drop(&h->_sync_group);
    h->_on_cancel = NULL;
    h->_arg = NULL;
    h->_on_drop = NULL;
}

static inline void _z_cancellation_token_on_cancel_handler_elem_clear(void* s) {
    _z_cancellation_token_on_cancel_handler_drop((_z_cancellation_token_on_cancel_handler_t*)s);
}

static inline void _z_cancellation_token_on_cancel_handler_move(_z_cancellation_token_on_cancel_handler_t* dst,
                                                                _z_cancellation_token_on_cancel_handler_t* src) {
    *dst = *src;
    *src = _z_cancellation_token_on_cancel_handler_null();
}

static inline void _z_cancellation_token_on_cancel_handler_elem_move(void* dst, void* src) {
    _z_cancellation_token_on_cancel_handler_move((_z_cancellation_token_on_cancel_handler_t*)dst,
                                                 (_z_cancellation_token_on_cancel_handler_t*)src);
}

_Z_SVEC_DEFINE_NO_COPY(_z_cancellation_token_on_cancel_handler, _z_cancellation_token_on_cancel_handler_t)

typedef struct {
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_t _mutex;
#endif
    _z_cancellation_token_on_cancel_handler_svec_t _handlers;
    bool _is_cancelled;
    z_result_t _cancel_result;
} _z_cancellation_token_t;

z_result_t _z_cancellation_token_create(_z_cancellation_token_t* ct);
bool _z_cancellation_token_is_cancelled(const _z_cancellation_token_t* ct);
z_result_t _z_cancellation_token_cancel(_z_cancellation_token_t* ct);
z_result_t _z_cancellation_token_cancel_with_timeout(_z_cancellation_token_t* ct, uint32_t timeout_ms);
void _z_cancellation_token_clear(_z_cancellation_token_t* ct);
z_result_t _z_cancellation_token_add_on_cancel_handler(_z_cancellation_token_t* ct,
                                                       _z_cancellation_token_on_cancel_handler_t* handler);

_Z_REFCOUNT_DEFINE(_z_cancellation_token, _z_cancellation_token)

#if Z_FEATURE_QUERY == 1
// Query cancellation
z_result_t _z_cancellation_token_add_on_query_cancel_handler(_z_cancellation_token_t* ct, const _z_session_rc_t* zs,
                                                             _z_zint_t qid);

#if Z_FEATURE_LIVELINESS == 1
// Liveliness Query cancellation
z_result_t _z_cancellation_token_add_on_liveliness_query_cancel_handler(_z_cancellation_token_t* ct,
                                                                        const _z_session_rc_t* zs, _z_zint_t qid);
#endif
#endif
#endif
