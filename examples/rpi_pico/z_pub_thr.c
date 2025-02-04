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
#include <zenoh-pico.h>

#include "config.h"

#if Z_FEATURE_PUBLICATION == 1

#define KEYEXPR "test/thr"
#define PAYLOAD_SIZE 8

void z_free_with_context(void *ptr, void *context) {
    (void)context;
    z_free(ptr);
}

void app_main(void) {
    uint8_t *value = (uint8_t *)z_malloc(PAYLOAD_SIZE);
    memset(value, 1, PAYLOAD_SIZE);

    // Set config
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, ZENOH_CONFIG_MODE);
    if (strcmp(ZENOH_CONFIG_CONNECT, "") != 0) {
        printf("Connect endpoint: %s\n", ZENOH_CONFIG_CONNECT);
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, ZENOH_CONFIG_CONNECT);
    }
    if (strcmp(ZENOH_CONFIG_LISTEN, "") != 0) {
        printf("Listen endpoint: %s\n", ZENOH_CONFIG_LISTEN);
        zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, ZENOH_CONFIG_LISTEN);
    }

    printf("Opening %s session ...\n", ZENOH_CONFIG_MODE);
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_drop(z_move(s));
        return;
    }

    // Declare publisher
    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) < 0) {
        printf("Unable to declare publisher for key expression!\n");
        return;
    }

    printf("Send packets\n");
    z_owned_bytes_t payload;
    z_bytes_from_buf(&payload, value, PAYLOAD_SIZE, z_free_with_context, NULL);
    while (1) {
        // Clone payload
        z_owned_bytes_t p;
        z_bytes_clone(&p, z_loan(payload));
        z_publisher_put(z_loan(pub), z_move(p), NULL);
    }

    // Clean-up
    z_drop(z_move(pub));
    z_drop(z_move(s));
    z_drop(z_move(payload));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
    return -2;
}
#endif
