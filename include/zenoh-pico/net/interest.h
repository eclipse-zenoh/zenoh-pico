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

#ifndef ZENOH_PICO_INTEREST_NETAPI_H
#define ZENOH_PICO_INTEREST_NETAPI_H

#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"

/**
 * Return type when declaring a queryable.
 */
typedef struct _z_interest_t {
    uint32_t _entity_id;
    _z_session_rc_t _zn;
} _z_interest_t;

#if Z_FEATURE_INTEREST == 1
void _z_interest_clear(_z_interest_t *intr);
void _z_interest_free(_z_interest_t **intr);
#endif

#endif /* ZENOH_PICO_INTEREST_NETAPI_H */
