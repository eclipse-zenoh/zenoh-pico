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

#ifndef ZENOH_PICO_SESSION_INTEREST_H
#define ZENOH_PICO_SESSION_INTEREST_H

#include <stdbool.h>

#include "zenoh-pico/net/interest.h"
#include "zenoh-pico/net/session.h"

#if Z_FEATURE_INTEREST == 1
#define _Z_INTEREST_RESOURCE_DEFAULT false
#define _Z_INTEREST_SUBSCRIBER_DEFAULT true
#define _Z_INTEREST_QUERYABLE_DEFAULT true
#define _Z_INTEREST_TOKEN_DEFAULT false
#define _Z_INTEREST_CURRENT_DEFAULT true
#define _Z_INTEREST_FUTURE_DEFAULT false
#define _Z_INTEREST_AGGREGATE_DEFAULT true

_z_session_interest_rc_t *_z_get_interest_by_id(_z_session_t *zn, const _z_zint_t id);
_z_session_interest_rc_list_t *_z_get_interest_by_key(_z_session_t *zn, const _z_keyexpr_t key);

_z_session_interest_rc_t *_z_register_interest(_z_session_t *zn, _z_session_interest_t *q);
int8_t _z_trigger_interests(_z_session_t *zn, const _z_msg_query_t *query, const _z_keyexpr_t q_key, uint32_t qid);
void _z_unregister_interest(_z_session_t *zn, _z_session_interest_rc_t *q);
void _z_flush_interest(_z_session_t *zn);
#endif  // Z_FEATURE_INTEREST == 1

#endif /* ZENOH_PICO_SESSION_INTEREST_H */
