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

#ifndef ZENOH_PICO_SESSION_QUERY_H
#define ZENOH_PICO_SESSION_QUERY_H

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"

#if Z_FEATURE_QUERY == 1
/*------------------ Query ------------------*/
_z_zint_t _z_get_query_id(_z_session_t *zn);

_z_pending_query_t *_z_get_pending_query_by_id(_z_session_t *zn, const _z_zint_t id);

int8_t _z_register_pending_query(_z_session_t *zn, _z_pending_query_t *pq);
int8_t _z_trigger_query_reply_partial(_z_session_t *zn, _z_zint_t reply_context, const _z_keyexpr_t keyexpr,
                                      _z_msg_put_t *msg, z_sample_kind_t kind);
int8_t _z_trigger_query_reply_err(_z_session_t *zn, _z_zint_t id, _z_msg_err_t *msg);
int8_t _z_trigger_query_reply_final(_z_session_t *zn, _z_zint_t id);
void _z_unregister_pending_query(_z_session_t *zn, _z_pending_query_t *pq);
void _z_flush_pending_queries(_z_session_t *zn);
#endif

#endif /* ZENOH_PICO_SESSION_QUERY_H */
