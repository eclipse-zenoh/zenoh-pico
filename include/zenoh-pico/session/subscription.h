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

#ifndef INCLUDE_ZENOH_PICO_SESSION_SUBSCRIPTION_H
#define INCLUDE_ZENOH_PICO_SESSION_SUBSCRIPTION_H

#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"

// Forward declaration to avoid cyclical include
typedef struct _z_session_t _z_session_t;

// Subscription infos
typedef struct {
    _z_sample_handler_t callback;
    void *arg;
} _z_subscription_infos_t;

_Z_ELEM_DEFINE(_z_subscription_infos, _z_subscription_infos_t, _z_noop_size, _z_noop_clear, _z_noop_copy, _z_noop_move)
_Z_SVEC_DEFINE(_z_subscription_infos, _z_subscription_infos_t, false)

typedef struct {
    _z_keyexpr_t ke_in;
    _z_keyexpr_t ke_out;
    _z_subscription_infos_svec_t infos;
} _z_subscription_cache_t;

/*------------------ Subscription ------------------*/
void _z_trigger_local_subscriptions(_z_session_t *zn, const _z_keyexpr_t *keyexpr, _z_bytes_t *payload,
                                    _z_encoding_t *encoding, const _z_n_qos_t qos, const _z_timestamp_t *timestamp,
                                    _z_bytes_t *attachment, z_reliability_t reliability);

#if Z_FEATURE_SUBSCRIPTION == 1

#if Z_FEATURE_RX_CACHE == 1
void _z_subscription_cache_clear(_z_subscription_cache_t *cache);
#endif

_z_subscription_rc_t *_z_get_subscription_by_id(_z_session_t *zn, uint8_t is_local, const _z_zint_t id);
_z_subscription_rc_t *_z_register_subscription(_z_session_t *zn, uint8_t is_local, _z_subscription_t *sub);
z_result_t _z_trigger_subscriptions(_z_session_t *zn, const _z_keyexpr_t *keyexpr, _z_bytes_t *payload,
                                    _z_encoding_t *encoding, const _z_zint_t kind, const _z_timestamp_t *timestamp,
                                    const _z_n_qos_t qos, _z_bytes_t *attachment, z_reliability_t reliability);
void _z_unregister_subscription(_z_session_t *zn, uint8_t is_local, _z_subscription_rc_t *sub);
void _z_flush_subscriptions(_z_session_t *zn);
#endif

#endif /* INCLUDE_ZENOH_PICO_SESSION_SUBSCRIPTION_H */
