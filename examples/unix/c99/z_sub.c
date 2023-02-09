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
//

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

void data_handler(const z_sample_t *sample, void *arg) {
    (void)(arg);
    z_owned_str_t keystr = z_keyexpr_to_string(sample->keyexpr);
    printf(">> [Subscriber] Received ('%s': '%.*s')\n", z_str_loan(&keystr), (int)sample->payload.len,
           sample->payload.start);
    z_str_drop(z_str_move(&keystr));
}

int main(int argc, char **argv) {
    const char *keyexpr = "demo/example/**";
    const char *mode = "client";
    char *locator = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'e':
                locator = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }

    z_owned_config_t config = z_config_default();
    zp_config_insert(z_config_loan(&config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    if (locator != NULL) {
        zp_config_insert(z_config_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(locator));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_config_move(&config));
    if (!z_session_check(&s)) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan(&s), NULL) < 0 || zp_start_lease_task(z_session_loan(&s), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        return -1;
    }

    z_owned_closure_sample_t callback = z_closure_sample(data_handler, NULL, NULL);
    printf("Declaring Subscriber on '%s'...\n", keyexpr);
    z_owned_subscriber_t sub =
        z_declare_subscriber(z_session_loan(&s), z_keyexpr(keyexpr), z_closure_sample_move(&callback), NULL);
    if (!z_subscriber_check(&sub)) {
        printf("Unable to declare subscriber.\n");
        return -1;
    }

    printf("Enter 'q' to quit...\n");
    char c = '\0';
    while (c != 'q') {
        fflush(stdin);
        scanf("%c", &c);
    }

    z_undeclare_subscriber(z_subscriber_move(&sub));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_session_loan(&s));
    zp_stop_lease_task(z_session_loan(&s));

    z_close(z_session_move(&s));

    return 0;
}
