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

#include "zenoh-pico/config.h"

#include <stddef.h>

#include "zenoh-pico/net/config.h"

_z_config_t *_z_config_empty(void) {
    _z_config_t *config = (_z_config_t *)z_malloc(sizeof(_z_config_t));
    if (config != NULL) {
        _z_config_init(config);
    }
    return config;
}

_z_config_t *_z_config_default(void) { return _z_config_client(NULL); }

_z_config_t *_z_config_client(const char *locator) {
    _z_config_t *ps = _z_config_empty();
    if (ps != NULL) {
        _zp_config_insert(ps, Z_CONFIG_MODE_KEY, _z_string_make(Z_CONFIG_MODE_CLIENT));
        if (locator != NULL) {
            // Connect only to the provided locator
            _zp_config_insert(ps, Z_CONFIG_PEER_KEY, _z_string_make(locator));
        } else {
            // The locator is not provided, we should perform scouting
            _zp_config_insert(ps, Z_CONFIG_MULTICAST_SCOUTING_KEY, _z_string_make(Z_CONFIG_MULTICAST_SCOUTING_DEFAULT));
            _zp_config_insert(ps, Z_CONFIG_MULTICAST_LOCATOR_KEY, _z_string_make(Z_CONFIG_MULTICAST_LOCATOR_DEFAULT));
            _zp_config_insert(ps, Z_CONFIG_SCOUTING_TIMEOUT_KEY, _z_string_make(Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT));
        }
    }
    return ps;
}
