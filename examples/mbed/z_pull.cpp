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

#include <EthernetInterface.h>
#include <mbed.h>
#include <randLIB.h>
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
//     printf(" >> [Subscriber handler] Received ('%s': '%.*s')\n", z_string_data(z_view_string_loan(&keystr)),
//     (int)sample->payload.len,
//            sample->payload.start);
// }

int main(int argc, char **argv) {
    randLIB_seed_random();

    EthernetInterface net;
    net.set_network("192.168.11.2", "255.255.255.0", "192.168.11.1");
    net.connect();

    // Initialize Zenoh Session and other parameters
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, MODE);
    if (strcmp(CONNECT, "") != 0) {
        zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, CONNECT);
    }

    // Open Zenoh session
    printf("Opening Zenoh Session...");
    z_owned_session_t s;
    if (z_open(&s, z_config_move(&config)) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    printf("OK\n");

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_session_loan_mut(&s), NULL);
    zp_start_lease_task(z_session_loan_mut(&s), NULL);

    // @TODO
    // z_owned_closure_sample_t callback;
    // z_closure_sample(&callback, data_handler, NULL, NULL);
    printf("Declaring Subscriber on '%s'...", KEYEXPR);
    // @TODO
    // z_owned_pull_subscriber_t sub =
    //     z_declare_pull_subscriber(z_session_loan(&s), z_loan(ke), z_closure_sample_move(&callback), NULL);
    // if (!z_pull_subscriber_check(&sub)) {
    //     printf("Unable to declare subscriber.\n");
    //     exit(-1);
    // }
    // printf("OK!\n");

    // while (1) {
    //     z_sleep_s(5);
    //     printf("Pulling data from '%s'...\n", KEYEXPR);
    //     z_subscriber_pull(z_pull_subscriber_loan(&sub));
    // }

    // printf("Closing Zenoh Session...");
    // z_undeclare_pull_subscriber(z_pull_subscriber_move(&sub));
    printf("Pull Subscriber not supported... exiting\n");

    z_close(z_session_move(&s));
    printf("OK!\n");

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
