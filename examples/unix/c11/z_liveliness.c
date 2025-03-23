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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_LIVELINESS == 1

int main(int argc, char **argv) {
    const char *keyexpr = "group1/zenoh-pico";
    const char *mode = "peer";
    const char *clocator = "serial/ttyACM0#baudrate=921600";
    const char *llocator = NULL;
    unsigned long timeout = 5;

    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:l:t:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'e':
                clocator = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case 'l':
                llocator = optarg;
                break;
            case 't':
                timeout = (unsigned long)atoi(optarg);
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm' || optopt == 'v' || optopt == 'l' ||
                    optopt == 't') {
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
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, mode);
    if (clocator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, clocator);
    }
    if (llocator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, llocator);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }

    // Wait for the serial port connection sequence to complete
    if (strcmp(mode, "peer") == 0) {
        printf("Waiting for startup...\n");
        usleep(100);
    }

    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression", keyexpr);
        return -1;
    }

    printf("Declaring liveliness token '%s'...\n", keyexpr);
    z_owned_liveliness_token_t token;
    if (z_liveliness_declare_token(z_loan(s), &token, z_loan(ke), NULL) < 0) {
        printf("Unable to create liveliness token!\n");
        exit(-1);
    }

    printf("Press CTRL-C to undeclare liveliness token and quit...\n");
    z_clock_t clock = z_clock_now();
    while (timeout == 0 || z_clock_elapsed_s(&clock) < timeout) {
        z_sleep_ms(100);
    }

    // LivelinessTokens are automatically closed when dropped
    // Use the code below to manually undeclare it if needed
    printf("Undeclaring liveliness token...\n");
    z_drop(z_move(token));

    z_drop(z_move(s));
    return 0;
}
#else
int main(void) {
    printf(
        "ERROR: Zenoh pico was compiled without Z_FEATURE_QUERY or Z_FEATURE_MULTI_THREAD but this example requires "
        "them.\n");
    return -2;
}
#endif
