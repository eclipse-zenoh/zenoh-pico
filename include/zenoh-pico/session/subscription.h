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

#include "zenoh-pico/net/session.h"

#if Z_FEATURE_SUBSCRIPTION == 1
/*------------------ Subscription ------------------*/
_z_subscription_sptr_t *_z_get_subscription_by_id(_z_session_t *zn, uint8_t is_local, const _z_zint_t id);
_z_subscription_sptr_list_t *_z_get_subscriptions_by_key(_z_session_t *zn, uint8_t is_local,
                                                         const _z_keyexpr_t *keyexpr);

_z_subscription_sptr_t *_z_register_subscription(_z_session_t *zn, uint8_t is_local, _z_subscription_t *sub);
void _z_trigger_local_subscriptions(_z_session_t *zn, const _z_keyexpr_t keyexpr, const uint8_t *payload,
                                    _z_zint_t payload_len, z_attachment_t att);
int8_t _z_trigger_subscriptions(_z_session_t *zn, const _z_keyexpr_t keyexpr, const _z_bytes_t payload,
                                const _z_encoding_t encoding, const _z_zint_t kind, const _z_timestamp_t timestamp,
                                z_attachment_t att);
void _z_unregister_subscription(_z_session_t *zn, uint8_t is_local, _z_subscription_sptr_t *sub);
void _z_flush_subscriptions(_z_session_t *zn);

/*------------------ Pull ------------------*/
_z_zint_t _z_get_pull_id(_z_session_t *zn);
#endif

#endif /* INCLUDE_ZENOH_PICO_SESSION_SUBSCRIPTION_H */
