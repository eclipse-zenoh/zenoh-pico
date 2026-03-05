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

#define KEYEXPR "demo/example/zenoh-pico-pub"
#define VALUE "[RT-Thread] Pub from Zenoh-Pico!"

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

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_move(s), NULL);
        return -1;
    }

    printf("Declaring Publisher on '%s'...\n", keyexpr);
    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, keyexpr);
    if (z_declare_publisher(&pub, z_loan(s), z_loan(ke), NULL) < 0) {
        printf("Unable to declare Publisher for key expression!\n");
        return -1;
    }

    char buf[256];
    for (int idx = 0; 1; ++idx) {
        rt_thread_mdelay(1000);
        snprintf(buf, sizeof(buf), "[%4d] %s", idx, value);
        printf("Putting Data ('%s': '%s')...\n", keyexpr, buf);

        z_owned_bytes_t payload;
        z_bytes_from_str(&payload, buf);
        z_publisher_put(z_loan(pub), z_move(payload), NULL);
    }

    // Clean up
    z_undeclare_publisher(z_move(pub));
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s), NULL);
    return 0;
}

// RT-Thread MSH command
static int zenoh_pub(int argc, char **argv) {
    return main(argc, argv);
}
MSH_CMD_EXPORT(zenoh_pub, zenoh publisher example);