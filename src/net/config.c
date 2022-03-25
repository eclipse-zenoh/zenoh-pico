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

#include "zenoh-pico/config.h"
#include "zenoh-pico/net/config.h"

_z_properties_t *_z_properties_empty()
{
    _z_properties_t *config = (_z_properties_t *)malloc(sizeof(_z_properties_t));
    _z_properties_init(config);
    return config;
}

_z_properties_t *_z_properties_default()
{
    return _z_properties_client(NULL);
}

_z_properties_t *_z_properties_client(const _z_str_t locator)
{
    _z_properties_t *ps = _z_properties_empty();
    _z_properties_insert(ps, Z_CONFIG_MODE_KEY, z_string_make(Z_CONFIG_MODE_CLIENT));
    if (locator)
    {
        // Connect only to the provided locator
        _z_properties_insert(ps, Z_CONFIG_PEER_KEY, z_string_make(locator));
    }
    else
    {
        // The locator is not provided, we should perform scouting
        _z_properties_insert(ps, Z_CONFIG_MULTICAST_SCOUTING_KEY, z_string_make(Z_CONFIG_MULTICAST_SCOUTING_DEFAULT));
        _z_properties_insert(ps, Z_CONFIG_MULTICAST_ADDRESS_KEY, z_string_make(Z_CONFIG_MULTICAST_ADDRESS_DEFAULT));
        _z_properties_insert(ps, Z_CONFIG_MULTICAST_INTERFACE_KEY, z_string_make(Z_CONFIG_MULTICAST_INTERFACE_DEFAULT));
        _z_properties_insert(ps, Z_CONFIG_SCOUTING_TIMEOUT_KEY, z_string_make(Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT));
    }
    return ps;
}
