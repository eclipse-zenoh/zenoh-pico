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

#include "zenoh-pico/api/primitives.h"

#if Z_FEATURE_SUBSCRIPTION == 1
int main(int argc, char **argv) {
    const char *keyexpr = "demo/example/**";
    char *locator = NULL;
    size_t interval = 5000;
    size_t size = 3;

    int opt;
    while ((opt = getopt(argc, argv, "k:e:i:s:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'e':
                locator = optarg;
                break;
            case 'i':
                interval = (size_t)atoi(optarg);
                break;
            case 's':
                size = (size_t)atoi(optarg);
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'i' || optopt == 's') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }

    z_owned_config_t config;
    z_config_default(&config);
    if (locator != NULL) {
        zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, locator);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_config_move(&config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan_mut(&s), NULL) < 0 || zp_start_lease_task(z_session_loan_mut(&s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }

    printf("Declaring Subscriber on '%s'...\n", keyexpr);
    z_owned_closure_sample_t closure;
    z_owned_ring_handler_sample_t handler;
    z_ring_channel_sample_new(&closure, &handler, size);
    z_owned_subscriber_t sub;
    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        return -1;
    }
    if (z_declare_subscriber(z_session_loan(&s), &sub, z_view_keyexpr_loan(&ke), z_closure_sample_move(&closure),
                             NULL) < 0) {
        printf("Unable to declare subscriber.\n");
        return -1;
    }

    printf("Pulling data every %zu ms... Ring size: %zd\n", interval, size);
    z_owned_sample_t sample;
    while (true) {
        z_result_t res;
        for (res = z_ring_handler_sample_try_recv(z_ring_handler_sample_loan(&handler), &sample); res == Z_OK;
             res = z_ring_handler_sample_try_recv(z_ring_handler_sample_loan(&handler), &sample)) {
            z_view_string_t keystr;
            z_keyexpr_as_view_string(z_sample_keyexpr(z_sample_loan(&sample)), &keystr);
            z_owned_string_t value;
            z_bytes_to_string(z_sample_payload(z_sample_loan(&sample)), &value);
            printf(">> [Subscriber] Pulled ('%.*s': '%.*s')\n", (int)z_string_len(z_view_string_loan(&keystr)),
                   z_string_data(z_view_string_loan(&keystr)), (int)z_string_len(z_string_loan(&value)),
                   z_string_data(z_string_loan(&value)));
            z_string_drop(z_string_move(&value));
            z_sample_drop(z_sample_move(&sample));
        }
        if (res == Z_CHANNEL_NODATA) {
            printf(">> [Subscriber] Nothing to pull... sleep for %zu ms\n", interval);
            z_sleep_ms(interval);
        } else {
            break;
        }
    }

    z_subscriber_drop(z_subscriber_move(&sub));
    z_ring_handler_sample_drop(z_ring_handler_sample_move(&handler));

    z_session_drop(z_session_move(&s));

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
