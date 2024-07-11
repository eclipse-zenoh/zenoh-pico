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
#include <zenoh-pico.h>

#if Z_FEATURE_SUBSCRIPTION == 1
// @TODO
// void data_handler(const z_loaned_sample_t *sample, void *ctx) {
//     (void)(ctx);
//     z_view_string_t keystr;
//     z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
//     printf(">> [Subscriber] Received ('%s': '%.*s')\n", z_string_data(z_loan(keystr)), (int)sample->payload.len,
//            sample->payload.start);
// }

int main(int argc, char **argv) {
    (void)(argc);
    (void)(argv);
    const char *keyexpr = "demo/example/**";
    char *locator = NULL;

    z_owned_config_t config;
    z_config_default(&config);
    if (locator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, locator);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config)) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
        return -1;
    }

    // @TODO
    // z_owned_closure_sample_t callback;
    // z_closure(&callback, data_handler);
    printf("Declaring Subscriber on '%s'...\n", keyexpr);
    // @TODO
    // z_owned_pull_subscriber_t sub = z_declare_pull_subscriber(z_loan(s), z_loan(ke), z_move(callback), NULL);
    // if (!z_check(sub)) {
    //     printf("Unable to declare subscriber.\n");
    //     return -1;
    // }

    // printf("Enter any key to pull data or 'q' to quit...\n");
    // char c = '\0';
    // while (1) {
    //     fflush(stdin);
    //     scanf("%c", &c);
    //     if (c == 'q') {
    //         break;
    //     }
    //     z_subscriber_pull(z_loan(sub));
    // }

    // z_undeclare_pull_subscriber(z_move(sub));
    printf("Pull Subscriber not supported... exiting\n");

    z_close(z_move(s));

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
