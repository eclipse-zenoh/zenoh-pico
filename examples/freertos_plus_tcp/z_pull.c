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
#define CONNECT "udp/224.0.0.225:7447#iface=en0"
#else
#error "Unknown Zenoh operation mode. Check CLIENT_OR_PEER value."
#endif

#define KEYEXPR "demo/example/**"

// @TODO
// void data_handler(const z_loaned_sample_t *sample, void *ctx) {
//     (void)(ctx);
//     z_view_string_t keystr;
//     z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
//     printf(">> [Subscriber] Received ('%s': '%.*s')\n", z_string_data(z_loan(keystr)), (int)sample->payload.len,
//            sample->payload.start);
// }

void app_main(void) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, MODE);
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

    // @TODO
    // z_owned_closure_sample_t callback;
    // z_closure(&callback, data_handler);
    printf("Declaring Subscriber on '%s'...\n", KEYEXPR);
    // @TODO
    // z_view_keyexpr_t ke;
    // z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    // z_owned_pull_subscriber_t sub = z_declare_pull_subscriber(z_loan(s), z_loan(ke), z_move(callback), NULL);
    // if (!z_check(sub)) {
    //     printf("Unable to declare subscriber.\n");
    //     return;
    // }

    // while (1) {
    //     z_sleep_s(5);
    //     printf("Pulling data from '%s'...\n", KEYEXPR);
    //     z_subscriber_pull(z_loan(sub));
    // }

    // z_undeclare_pull_subscriber(z_move(sub));
    printf("Pull Subscriber not supported... exiting\n");

    z_close(z_move(s));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
}
#endif
