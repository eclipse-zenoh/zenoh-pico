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

#include <stdio.h>
#include <stdlib.h>
#include <zenoh-pico.h>

#if Z_FEATURE_SUBSCRIPTION == 1
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
// void data_handler(const z_loaned_sample_t *sample, void *arg) {
//     z_view_string_t keystr;
//     z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
//     printf(" >> [Subscriber handler] Received ('%s': '%.*s')\n", z_string_data(z_loan(keystr)),
//     (int)sample->payload.len,
//            sample->payload.start);
// }

int main(int argc, char **argv) {
    sleep(5);

    // Initialize Zenoh Session and other parameters
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
    if (strcmp(CONNECT, "") != 0) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, CONNECT);
    }

    // Open Zenoh session
    printf("Opening Zenoh Session...");
    z_owned_session_t s;
    if (z_open(&s, z_move(config)) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    printf("OK\n");

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_loan_mut(s), NULL);
    zp_start_lease_task(z_loan_mut(s), NULL);

    // @TODO
    // z_owned_closure_sample_t callback;
    // z_closure(&callback, data_handler);
    printf("Declaring Subscriber on '%s'...", KEYEXPR);
    // @TODO
    // z_owned_pull_subscriber_t sub = z_declare_pull_subscriber(z_loan(s), z_loan(ke), z_move(callback), NULL);
    // if (!z_check(sub)) {
    //     printf("Unable to declare subscriber.\n");
    //     exit(-1);
    // }
    // printf("OK!\n");

    // while (1) {
    //     sleep(5);
    //     printf("Pulling data from '%s'...\n", KEYEXPR);
    //     z_subscriber_pull(z_loan(sub));
    // }

    // printf("Closing Zenoh Session...");
    // z_undeclare_pull_subscriber(z_move(sub));
    printf("Pull Subscriber not supported... exiting\n");

    // Stop the receive and the session lease loop for zenoh-pico
    zp_stop_read_task(z_loan_mut(s));
    zp_stop_lease_task(z_loan_mut(s));

    z_close(z_move(s));
    printf("OK!\n");

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
