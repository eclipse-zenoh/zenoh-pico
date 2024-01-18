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

void data_handler(const z_sample_t *sample, void *ctx) {
    (void)(ctx);
    z_owned_str_t keystr = z_keyexpr_to_string(sample->keyexpr);
    printf(">> [Subscriber] Received ('%s': '%.*s')\n", z_loan(keystr), (int)sample->payload.len,
           sample->payload.start);
    z_drop(z_move(keystr));
}

void app_main(void) {
    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(MODE));
    if (strcmp(CONNECT, "") != 0) {
        zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, z_string_make(CONNECT));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
        return;
    }

    z_owned_closure_sample_t callback = z_closure(data_handler);
    printf("Declaring Subscriber on '%s'...\n", KEYEXPR);
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(s), z_keyexpr(KEYEXPR), z_move(callback), NULL);
    if (!z_check(sub)) {
        printf("Unable to declare subscriber.\n");
        return;
    }

    while (1) {
        zp_sleep_s(5);
    }

    z_undeclare_subscriber(z_move(sub));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));

    z_close(z_move(s));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
}
#endif
