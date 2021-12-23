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
#include "zenoh-pico/api/config.h"

zn_properties_t *zn_config_empty()
{
    zn_properties_t *config = (zn_properties_t *)malloc(sizeof(zn_properties_t));
    zn_properties_init(config);
    return config;
}

zn_properties_t *zn_config_default()
{
    return zn_config_client(NULL);
}

zn_properties_t *zn_config_client(const z_str_t locator)
{
    zn_properties_t *ps = zn_config_empty();
    zn_properties_insert(ps, ZN_CONFIG_MODE_KEY, z_string_make(ZN_CONFIG_MODE_CLIENT));
    if (locator)
    {
        // Connect only to the provided locator
        zn_properties_insert(ps, ZN_CONFIG_PEER_KEY, z_string_make(locator));
    }
    else
    {
        // The locator is not provided, we should perform scouting
        zn_properties_insert(ps, ZN_CONFIG_MULTICAST_SCOUTING_KEY, z_string_make(ZN_CONFIG_MULTICAST_SCOUTING_DEFAULT));
        zn_properties_insert(ps, ZN_CONFIG_MULTICAST_ADDRESS_KEY, z_string_make(ZN_CONFIG_MULTICAST_ADDRESS_DEFAULT));
        zn_properties_insert(ps, ZN_CONFIG_MULTICAST_INTERFACE_KEY, z_string_make(ZN_CONFIG_MULTICAST_INTERFACE_DEFAULT));
        zn_properties_insert(ps, ZN_CONFIG_SCOUTING_TIMEOUT_KEY, z_string_make(ZN_CONFIG_SCOUTING_TIMEOUT_DEFAULT));
    }
    return ps;
}
