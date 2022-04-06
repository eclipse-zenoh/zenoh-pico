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

#ifndef ZENOH_PICO_CONFIG_API_H
#define ZENOH_PICO_CONFIG_API_H

#include "zenoh-pico/utils/properties.h"
#include "zenoh-pico/collections/string.h"

/**
 * Create an empty set of properties for zenoh-net session configuration.
 *
 * Returns:
 *     A :c:type:`zn_properties_t` containing an empty configuration.
 */
zn_properties_t *zn_config_empty(void);

/**
 * Create a default set of properties for zenoh-net session configuration.
 *
 * Returns:
 *     A :c:type:`zn_properties_t` containing a default configuration.
 */
zn_properties_t *zn_config_default(void);

/**
 * Create a default set of properties for client mode zenoh-net session configuration.
 * If peer is not null, it is added to the configuration as remote peer.
 *
 * Parameters:
 *   locator: An optional peer locator. The caller keeps its ownership.
 * Returns:
 *     A :c:type:`zn_properties_t` containing a default configuration for client mode.
 */
zn_properties_t *zn_config_client(const z_str_t locator);

#endif /* ZENOH_PICO_CONFIG_API_H */
