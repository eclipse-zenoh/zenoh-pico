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
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <zenoh-pico.h>

#if Z_FEATURE_PUBLICATION == 1
#define CLIENT_OR_PEER 0  // 0: Client mode; 1: Peer mode
#if CLIENT_OR_PEER == 0
#define MODE "client"
#define CONNECT ""  // If empty, it will scout
#elif CLIENT_OR_PEER == 1
#define MODE "peer"
#define CONNECT "udp/224.0.0.225:7447"
#else
#error "Unknown Zenoh operation mode. Check CLIENT_OR_PEER value."
#endif

#define KEYEXPR "demo/example/zenoh-pico-put"
#define VALUE "[FreeRTOS-Plus-TCP] Pub from Zenoh-Pico!"

void app_main(void) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
    if (strcmp(CONNECT, "") != 0) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, CONNECT);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config)) < 0) {
        printf("Unable to open session!\n");
        return;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
        return;
    }

    printf("Declaring key expression '%s'...\n", KEYEXPR);
    z_owned_keyexpr_t ke;
    z_view_keyexpr_t vke;
    z_view_keyexpr_from_str_unchecked(&vke, KEYEXPR);
    if (z_declare_keyexpr(&ke, z_loan(s), z_loan(vke)) < 0) {
        printf("Unable to declare key expression!\n");
        z_close(z_move(s));
        return;
    }

    printf("Putting Data ('%s': '%s')...\n", KEYEXPR, VALUE);
    z_put_options_t options;
    z_put_options_default(&options);

    // Create payload
    z_owned_bytes_t payload;
    z_bytes_serialize_from_str(&payload, VALUE);

    if (z_put(z_loan(s), z_loan(ke), z_move(payload), &options) < 0) {
        printf("Oh no! Put has failed...\n");
    }

    while (1) {
        z_sleep_s(1);
    }

    // Clean up
    z_undeclare_keyexpr(z_move(ke), z_loan(s));
    z_close(z_move(s));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
}
#endif
