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

#ifndef ZENOH_PICO_SESSION_QUERYABLE_H
#define ZENOH_PICO_SESSION_QUERYABLE_H

#include <stdbool.h>

#include "zenoh-pico/net/session.h"

#if Z_FEATURE_QUERYABLE == 1
#define _Z_QUERYABLE_COMPLETE_DEFAULT false
#define _Z_QUERYABLE_DISTANCE_DEFAULT 0

/*------------------ Queryable ------------------*/
_z_session_queryable_rc_t *_z_get_session_queryable_by_id(_z_session_t *zn, const _z_zint_t id);
_z_session_queryable_rc_list_t *_z_get_session_queryable_by_key(_z_session_t *zn, const _z_keyexpr_t key);

_z_session_queryable_rc_t *_z_register_session_queryable(_z_session_t *zn, _z_session_queryable_t *q);
int8_t _z_trigger_queryables(_z_session_rc_t *zn, _z_msg_query_t *query, const _z_keyexpr_t q_key, uint32_t qid,
                             const _z_bytes_t attachment);
void _z_unregister_session_queryable(_z_session_t *zn, _z_session_queryable_rc_t *q);
void _z_flush_session_queryable(_z_session_t *zn);
#endif

#endif /* ZENOH_PICO_SESSION_QUERYABLE_H */
