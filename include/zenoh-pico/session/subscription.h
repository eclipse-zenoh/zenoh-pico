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
#include "zenoh-pico/net/session.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------ Subscription ------------------*/
z_result_t _z_trigger_subscriptions_put(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_bytes_t payload,
                                        _z_encoding_t *encoding, const _z_timestamp_t *timestamp, const _z_n_qos_t qos,
                                        const _z_bytes_t attachment, z_reliability_t reliability);

z_result_t _z_trigger_subscriptions_del(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_timestamp_t *timestamp,
                                        const _z_n_qos_t qos, const _z_bytes_t attachment, z_reliability_t reliability);

z_result_t _z_trigger_liveliness_subscriptions_declare(_z_session_t *zn, const _z_keyexpr_t keyexpr,
                                                       const _z_timestamp_t *timestamp);

z_result_t _z_trigger_liveliness_subscriptions_undeclare(_z_session_t *zn, const _z_keyexpr_t keyexpr,
                                                         const _z_timestamp_t *timestamp);

#if Z_FEATURE_SUBSCRIPTION == 1
_z_subscription_rc_t *_z_get_subscription_by_id(_z_session_t *zn, _z_subscriber_kind_t kind, const _z_zint_t id);
_z_subscription_rc_list_t *_z_get_subscriptions_by_key(_z_session_t *zn, _z_subscriber_kind_t kind,
                                                       const _z_keyexpr_t *keyexpr);

_z_subscription_rc_t *_z_register_subscription(_z_session_t *zn, _z_subscriber_kind_t kind, _z_subscription_t *sub);
z_result_t _z_trigger_subscriptions_impl(_z_session_t *zn, _z_subscriber_kind_t subscriber_kind,
                                         const _z_keyexpr_t keyexpr, const _z_bytes_t payload, _z_encoding_t *encoding,
                                         const _z_zint_t sample_kind, const _z_timestamp_t *timestamp,
                                         const _z_n_qos_t qos, const _z_bytes_t attachment,
                                         z_reliability_t reliability);
void _z_unregister_subscription(_z_session_t *zn, _z_subscriber_kind_t kind, _z_subscription_rc_t *sub);
void _z_flush_subscriptions(_z_session_t *zn);
#endif

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_SESSION_SUBSCRIPTION_H */
