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
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rtthread.h>

#include "zenoh-pico.h"

#define KEYEXPR "demo/example/zenoh-pico-put"
#define VALUE "[RT-Thread] Put from Zenoh-Pico!"

int main(int argc, char **argv) {
    const char *keyexpr = KEYEXPR;
    const char *value = VALUE;
    const char *mode = "client";
    char *locator = NULL;

    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, mode);
    if (locator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, locator);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    printf("Putting Data ('%s': '%s')...\n", keyexpr, value);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, keyexpr);
    z_owned_bytes_t payload;
    z_bytes_from_str(&payload, value);
    z_put(z_loan(s), z_loan(ke), z_move(payload), NULL);

    z_close(z_move(s), NULL);
    return 0;
}

// RT-Thread MSH command
static int zenoh_put(int argc, char **argv) {
    return main(argc, argv);
}
MSH_CMD_EXPORT(zenoh_put, zenoh put example);