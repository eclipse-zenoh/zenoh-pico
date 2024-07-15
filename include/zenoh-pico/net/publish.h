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

#ifndef INCLUDE_ZENOH_PICO_NET_PUBLISH_H
#define INCLUDE_ZENOH_PICO_NET_PUBLISH_H

#include "zenoh-pico/net/filtering.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"

/**
 * Return type when declaring a publisher.
 */
typedef struct _z_publisher_t {
    _z_keyexpr_t _key;
    _z_zint_t _id;
    _z_session_rc_t _zn;
    z_congestion_control_t _congestion_control;
    z_priority_t _priority;
    _Bool _is_express;
#if Z_FEATURE_INTEREST == 1
    _z_write_filter_t _filter;
#endif
} _z_publisher_t;

#if Z_FEATURE_PUBLICATION == 1
void _z_publisher_clear(_z_publisher_t *pub);
void _z_publisher_free(_z_publisher_t **pub);
_Bool _z_publisher_check(const _z_publisher_t *publisher);
_z_publisher_t _z_publisher_null(void);
#endif

#endif /* INCLUDE_ZENOH_PICO_NET_PUBLISH_H */
