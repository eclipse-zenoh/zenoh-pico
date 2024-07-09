//
// Copyright (c) 2024 ZettaScale Technology
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"

#undef NDEBUG
#include <assert.h>

void config_client(void) {
    const char *locator = "tcp/127.0.0.1";
    z_owned_config_t config;
    assert(0 == z_config_client(&config, locator));
    assert(z_config_check(&config));
    strcmp(zp_config_get(z_config_loan(&config), Z_CONFIG_MODE_KEY), Z_CONFIG_MODE_CLIENT);
    strcmp(zp_config_get(z_config_loan(&config), Z_CONFIG_CONNECT_KEY), locator);
    z_config_drop(&config);
}

void config_peer(void) {
    const char *locator = "udp/228.0.0.4:7447#iface=en0";
    z_owned_config_t config;
    assert(0 == z_config_peer(&config, locator));
    assert(z_config_check(&config));
    strcmp(zp_config_get(z_config_loan(&config), Z_CONFIG_MODE_KEY), Z_CONFIG_MODE_PEER);
    strcmp(zp_config_get(z_config_loan(&config), Z_CONFIG_CONNECT_KEY), locator);
    z_config_drop(&config);
    assert(0 != z_config_peer(&config, NULL));
    assert(!z_config_check(&config));
}

int main(void) {
    config_client();
    config_peer();
}
