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
#include <string.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_PUBLICATION == 1

#if Z_FEATURE_MATCHING == 1
void matching_status_handler(const z_matching_status_t *matching_status, void *arg) {
    (void)arg;
    if (matching_status->matching) {
        printf("Publisher has matching subscribers.\n");
    } else {
        printf("Publisher has NO MORE matching subscribers.\n");
    }
}
#endif

int main(int argc, char **argv) {
    const char *keyexpr = "demo/example/zenoh-pico-pub";
    char *const default_value = "Pub from Pico!";
    char *value = default_value;
    const char *mode = "peer";
    char *clocator = "serial/ttyACM0#baudrate=921600";
    char *llocator = NULL;
    int n = 2147483647;  // max int value by default
    bool add_matching_listener = false;

    int opt;
    while ((opt = getopt(argc, argv, "k:v:e:m:l:n:a")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'v':
                value = optarg;
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
            case 'n':
                n = atoi(optarg);
                break;
            case 'a':
                add_matching_listener = true;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'v' || optopt == 'e' || optopt == 'm' || optopt == 'l' ||
                    optopt == 'n') {
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
        z_drop(z_move(s));
        return -1;
    }
    // Wait for the serial port connection sequence to complete
    if (strcmp(mode, "peer") == 0) {
        printf("Waiting for startup...\n");
        usleep(100000);
    }
    // Declare publisher
    printf("Declaring publisher for '%s'...\n", keyexpr);
    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        return -1;
    }
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) < 0) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }

    if (add_matching_listener) {
#if Z_FEATURE_MATCHING == 1
        z_owned_closure_matching_status_t callback;
        z_closure(&callback, matching_status_handler, NULL, NULL);
        z_publisher_declare_background_matching_listener(z_loan(pub), z_move(callback));
#else
        printf("ERROR: Zenoh pico was compiled without Z_FEATURE_MATCHING but this example requires it.\n");
        return -2;
#endif
    }

    // Publish data
    printf("Press CTRL-C to quit...\n");
    char buf[256];
    for (int idx = 0; idx < n; ++idx) {
        z_sleep_s(1);
        sprintf(buf, "[%4d] %s", idx, value);
        printf("Putting Data ('%s': '%s')...\n", keyexpr, buf);

        // Create payload
        z_owned_bytes_t payload;
        z_bytes_copy_from_str(&payload, buf);

        z_publisher_put(z_loan(pub), z_move(payload), NULL);
    }
    // Clean up
    z_drop(z_move(pub));
    z_drop(z_move(s));
    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
    return -2;
}
#endif
