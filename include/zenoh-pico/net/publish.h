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

#ifndef ZENOH_PICO_PUBLISH_NETAPI_H
#define ZENOH_PICO_PUBLISH_NETAPI_H

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"

/**
 * Return type when declaring a publisher.
 */
typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _id;
    _z_session_t *_zn;
    z_congestion_control_t _congestion_control;
    z_priority_t _priority;
} _z_publisher_t;

void _z_publisher_clear(_z_publisher_t *pub);
void _z_publisher_free(_z_publisher_t **pub);

#endif /* ZENOH_PICO_PUBLISH_NETAPI_H */
