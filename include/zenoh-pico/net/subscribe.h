/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef ZENOH_PICO_SUBSCRIBE_NETAPI_H
#define ZENOH_PICO_SUBSCRIBE_NETAPI_H

#include "zenoh-pico/protocol/core.h"

/**
 * Return type when declaring a subscriber.
 */
typedef struct
{
    void *_zn;  // FIXME: _z_session_t *zn;
    _z_zint_t _id;
} z_subscriber_t;

/**
 * Create a default subscription info.
 *
 * Returns:
 *     A :c:type:`_z_subinfo_t` containing the created subscription info.
 */
_z_subinfo_t _z_subinfo_default(void);

#endif /* ZENOH_PICO_SUBSCRIBE_NETAPI_H */
