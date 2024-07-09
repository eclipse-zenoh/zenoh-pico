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
// @TODO
// void data_handler(const z_loaned_sample_t *sample, void *ctx) {
//     (void)(ctx);
//     z_view_string_t keystr;
//     z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
//     printf(">> [Subscriber] Received ('%s': '%.*s')\n", z_string_data(z_view_string_loan(&keystr)),
//     (int)sample->payload.len,
//            sample->payload.start);
// }

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

    z_owned_config_t config;
    z_config_default(&config);
    if (locator != NULL) {
        zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, locator);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_config_move(&config)) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan_mut(&s), NULL) < 0 || zp_start_lease_task(z_session_loan_mut(&s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
        return -1;
    }

    // @TODO
    // z_owned_closure_sample_t callback;
    // z_closure_sample(&callback, data_handler, NULL, NULL);
    printf("Declaring Subscriber on '%s'...\n", keyexpr);
    // @TODO
    // z_owned_pull_subscriber_t sub =
    //     z_declare_pull_subscriber(z_session_loan(&s), z_loan(ke), z_closure_sample_move(&callback), NULL);
    // if (!z_pull_subscriber_check(&sub)) {
    //     printf("Unable to declare subscriber.\n");
    //     return -1;
    // }

    // printf("Enter any key to pull data or 'q' to quit...\n");
    // char c = '\0';
    // while (1) {
    //     fflush(stdin);
    //     int ret = scanf("%c", &c);
    //     (void)ret;  // Clear unused result warning
    //     if (c == 'q') {
    //         break;
    //     }
    //     z_subscriber_pull(z_pull_subscriber_loan(&sub));
    // }

    // z_undeclare_pull_subscriber(z_pull_subscriber_move(&sub));
    printf("Pull Subscriber not supported... exiting\n");

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_session_loan_mut(&s));
    zp_stop_lease_task(z_session_loan_mut(&s));

    z_close(z_session_move(&s));

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
