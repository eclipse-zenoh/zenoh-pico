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

#include "zenoh-pico/collections/lru_cache.h"
#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration to avoid cyclical include
typedef struct _z_session_t _z_session_t;

// Subscription infos
typedef struct {
    _z_closure_sample_callback_t callback;
    void *arg;
} _z_subscription_infos_t;

_Z_ELEM_DEFINE(_z_subscription_infos, _z_subscription_infos_t, _z_noop_size, _z_noop_clear, _z_noop_copy, _z_noop_move)
_Z_SVEC_DEFINE(_z_subscription_infos, _z_subscription_infos_t)

typedef struct {
    _z_keyexpr_t ke_in;
    _z_keyexpr_t ke_out;
    _z_subscription_infos_svec_t infos;
    size_t sub_nb;
} _z_subscription_cache_data_t;

void _z_subscription_cache_invalidate(_z_session_t *zn);
int _z_subscription_cache_data_compare(const void *first, const void *second);
void _z_subscription_cache_data_clear(_z_subscription_cache_data_t *val);

/*------------------ Subscription ------------------*/
z_result_t _z_trigger_subscriptions_put(_z_session_t *zn, _z_keyexpr_t *keyexpr, _z_bytes_t *payload,
                                        _z_encoding_t *encoding, const _z_timestamp_t *timestamp, const _z_n_qos_t qos,
                                        _z_bytes_t *attachment, z_reliability_t reliability,
                                        _z_source_info_t *source_info);

z_result_t _z_trigger_subscriptions_del(_z_session_t *zn, _z_keyexpr_t *keyexpr, const _z_timestamp_t *timestamp,
                                        const _z_n_qos_t qos, _z_bytes_t *attachment, z_reliability_t reliability,
                                        _z_source_info_t *source_info);

z_result_t _z_trigger_liveliness_subscriptions_declare(_z_session_t *zn, _z_keyexpr_t *keyexpr,
                                                       const _z_timestamp_t *timestamp);

z_result_t _z_trigger_liveliness_subscriptions_undeclare(_z_session_t *zn, _z_keyexpr_t *keyexpr,
                                                         const _z_timestamp_t *timestamp);

#if Z_FEATURE_SUBSCRIPTION == 1

#if Z_FEATURE_RX_CACHE == 1
_Z_ELEM_DEFINE(_z_subscription, _z_subscription_cache_data_t, _z_noop_size, _z_subscription_cache_data_clear,
               _z_noop_copy, _z_noop_move)
_Z_LRU_CACHE_DEFINE(_z_subscription, _z_subscription_cache_data_t, _z_subscription_cache_data_compare)
#endif

_z_subscription_rc_t *_z_get_subscription_by_id(_z_session_t *zn, _z_subscriber_kind_t kind, const _z_zint_t id);
_z_subscription_rc_t *_z_register_subscription(_z_session_t *zn, _z_subscriber_kind_t kind, _z_subscription_t *sub);
z_result_t _z_trigger_subscriptions_impl(_z_session_t *zn, _z_subscriber_kind_t sub_kind, _z_keyexpr_t *keyexpr,
                                         _z_bytes_t *payload, _z_encoding_t *encoding, const _z_zint_t sample_kind,
                                         const _z_timestamp_t *timestamp, const _z_n_qos_t qos, _z_bytes_t *attachment,
                                         z_reliability_t reliability, _z_source_info_t *source_info);
void _z_unregister_subscription(_z_session_t *zn, _z_subscriber_kind_t kind, _z_subscription_rc_t *sub);
void _z_flush_subscriptions(_z_session_t *zn);
#endif

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_SESSION_SUBSCRIPTION_H */
