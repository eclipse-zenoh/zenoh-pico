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

#if Z_FEATURE_SUBSCRIPTION == 1

#define KEYEXPR "demo/example/**"

const size_t INTERVAL = 5000;
const size_t SIZE = 3;

void app_main(void) {
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

    printf("Declaring Subscriber on '%s'...\n", KEYEXPR);
    z_owned_closure_sample_t closure;
    z_owned_ring_handler_sample_t handler;
    z_ring_channel_sample_new(&closure, &handler, SIZE);
    z_owned_subscriber_t sub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    if (z_declare_subscriber(z_loan(s), &sub, z_loan(ke), z_move(closure), NULL) < 0) {
        printf("Unable to declare subscriber.\n");
        return;
    }

    printf("Pulling data every %zu ms... Ring size: %zd\n", INTERVAL, SIZE);
    z_owned_sample_t sample;
    while (true) {
        z_result_t res;
        for (res = z_try_recv(z_loan(handler), &sample); res == Z_OK; res = z_try_recv(z_loan(handler), &sample)) {
            z_view_string_t keystr;
            z_keyexpr_as_view_string(z_sample_keyexpr(z_loan(sample)), &keystr);
            z_owned_string_t value;
            z_bytes_to_string(z_sample_payload(z_loan(sample)), &value);
            printf(">> [Subscriber] Pulled ('%.*s': '%.*s')\n", (int)z_string_len(z_loan(keystr)),
                   z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(value)), z_string_data(z_loan(value)));
            z_drop(z_move(value));
            z_drop(z_move(sample));
        }
        if (res == Z_CHANNEL_NODATA) {
            printf(">> [Subscriber] Nothing to pull... sleep for %zu ms\n", INTERVAL);
            z_sleep_ms(INTERVAL);
        } else {
            break;
        }
    }

    z_drop(z_move(sub));
    z_drop(z_move(handler));

    z_drop(z_move(s));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
}
#endif
