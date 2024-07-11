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

/*------------------ Subscription ------------------*/
void _z_trigger_local_subscriptions(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_bytes_t payload,
                                    const _z_n_qos_t qos, const _z_bytes_t attachment);

#if Z_FEATURE_SUBSCRIPTION == 1
_z_subscription_rc_t *_z_get_subscription_by_id(_z_session_t *zn, uint8_t is_local, const _z_zint_t id);
_z_subscription_rc_list_t *_z_get_subscriptions_by_key(_z_session_t *zn, uint8_t is_local, const _z_keyexpr_t *keyexpr);

_z_subscription_rc_t *_z_register_subscription(_z_session_t *zn, uint8_t is_local, _z_subscription_t *sub);
int8_t _z_trigger_subscriptions(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_bytes_t payload,
                                _z_encoding_t *encoding, const _z_zint_t kind, const _z_timestamp_t *timestamp,
                                const _z_n_qos_t qos, const _z_bytes_t attachment);
void _z_unregister_subscription(_z_session_t *zn, uint8_t is_local, _z_subscription_rc_t *sub);
void _z_flush_subscriptions(_z_session_t *zn);
#endif

#endif /* INCLUDE_ZENOH_PICO_SESSION_SUBSCRIPTION_H */
