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

#ifndef ZENOH_PICO_SUBSCRIBE_NETAPI_H
#define ZENOH_PICO_SUBSCRIBE_NETAPI_H

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"

/**
 * Return type when declaring a subscriber.
 */
typedef struct {
    _z_zint_t _id;
    _z_session_t *_zn;
} _z_subscriber_t;

typedef _z_subscriber_t _z_pull_subscriber_t;

/**
 * Create a default subscription info for a push subscriber.
 *
 * Returns:
 *     A :c:type:`_z_subinfo_t` containing the created subscription info.
 */
_z_subinfo_t _z_subinfo_push_default(void);

/**
 * Create a default subscription info for a pull subscriber.
 *
 * Returns:
 *     A :c:type:`_z_subinfo_t` containing the created subscription info.
 */
_z_subinfo_t _z_subinfo_pull_default(void);

void _z_subscriber_clear(_z_subscriber_t *sub);
void _z_subscriber_free(_z_subscriber_t **sub);

#endif /* ZENOH_PICO_SUBSCRIBE_NETAPI_H */
