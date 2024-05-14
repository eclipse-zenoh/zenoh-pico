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

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_SUBSCRIPTION == 1
int main(int argc, char **argv) {
    const char *keyexpr = "demo/example/**";
    char *locator = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "k:e:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'e':
                locator = optarg;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e') {
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
    if (locator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, z_string_make(locator));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
        return -1;
    }

    printf("Declaring Subscriber on '%s'...\n", keyexpr);
    z_owned_sample_fifo_channel_t channel = z_sample_fifo_channel_new(3);
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(s), z_keyexpr(keyexpr), z_move(channel.send), NULL);
    if (!z_check(sub)) {
        printf("Unable to declare subscriber.\n");
        return -1;
    }

    z_owned_sample_t sample = z_sample_null();
    for (z_call(channel.recv, &sample); z_check(sample); z_call(channel.recv, &sample)) {
        z_sample_t loaned_sample = z_loan(sample);
        z_keyexpr_t sample_key = z_sample_keyexpr(&loaned_sample);
        z_bytes_t payload = z_sample_payload(&loaned_sample);
        z_owned_str_t keystr = z_keyexpr_to_string(sample_key);
        printf(">> [Subscriber] Received ('%s': '%.*s')\n", z_loan(keystr), (int)payload.len, payload.start);
        z_drop(z_move(keystr));
        z_drop(z_move(sample));
        sample = z_sample_null();
    }

    z_undeclare_subscriber(z_move(sub));
    z_drop(z_move(channel));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));

    z_close(z_move(s));

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
