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

#ifndef ZENOH_PICO_SESSION_LIVELINESS_H
#define ZENOH_PICO_SESSION_LIVELINESS_H

#include "zenoh-pico/session/session.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_LIVELINESS == 1
typedef struct _z_session_t _z_session_t;

typedef struct {
    _z_keyexpr_t _key;
    _z_closure_reply_callback_t _callback;
    _z_drop_handler_t _dropper;
    void *_arg;
} _z_liveliness_pending_query_t;

void _z_liveliness_pending_query_clear(_z_liveliness_pending_query_t *res);
void _z_liveliness_pending_query_copy(_z_liveliness_pending_query_t *dst, const _z_liveliness_pending_query_t *src);
_z_liveliness_pending_query_t *_z_liveliness_pending_query_clone(const _z_liveliness_pending_query_t *src);

_Z_ELEM_DEFINE(_z_liveliness_pending_query, _z_liveliness_pending_query_t, _z_noop_size,
               _z_liveliness_pending_query_clear, _z_liveliness_pending_query_copy, _z_noop_move)
_Z_INT_MAP_DEFINE(_z_liveliness_pending_query, _z_liveliness_pending_query_t)

uint32_t _z_liveliness_get_query_id(_z_session_t *zn);

z_result_t _z_liveliness_register_token(_z_session_t *zn, uint32_t id, const _z_keyexpr_t *keyexpr);
void _z_liveliness_unregister_token(_z_session_t *zn, uint32_t id);

#if Z_FEATURE_SUBSCRIPTION == 1
z_result_t _z_liveliness_subscription_declare(_z_session_t *zn, uint32_t id, const _z_keyexpr_t *keyexpr,
                                              const _z_timestamp_t *timestamp);
z_result_t _z_liveliness_subscription_undeclare(_z_session_t *zn, uint32_t id, const _z_timestamp_t *timestamp);
z_result_t _z_liveliness_subscription_undeclare_all(_z_session_t *zn);
z_result_t _z_liveliness_subscription_trigger_history(_z_session_t *zn, const _z_keyexpr_t *keyexpr);
#endif

#if Z_FEATURE_QUERY == 1
z_result_t _z_liveliness_register_pending_query(_z_session_t *zn, uint32_t id, _z_liveliness_pending_query_t *pen_qry);
void _z_liveliness_unregister_pending_query(_z_session_t *zn, uint32_t id);
#endif

z_result_t _z_liveliness_process_token_declare(_z_session_t *zn, const _z_n_msg_declare_t *decl);
z_result_t _z_liveliness_process_token_undeclare(_z_session_t *zn, const _z_n_msg_declare_t *decl);
z_result_t _z_liveliness_process_declare_final(_z_session_t *zn, const _z_n_msg_declare_t *decl);

void _z_liveliness_init(_z_session_t *zn);
void _z_liveliness_clear(_z_session_t *zn);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_SESSION_LIVELINESS_H */
