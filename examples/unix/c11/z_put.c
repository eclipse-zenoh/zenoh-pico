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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/api/types.h"

#if Z_FEATURE_PUBLICATION == 1

static int parse_args(int argc, char **argv, z_owned_config_t *config, char **keyexpr, char **value);

int main(int argc, char **argv) {
    char *keyexpr = "demo/example/zenoh-pico-put";
    char *const default_value = "Pub from Pico!";
    char *value = default_value;

    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config, &keyexpr, &value);
    if (ret != 0) {
        return ret;
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

    printf("Declaring key expression '%s'...\n", keyexpr);
    z_view_keyexpr_t vke;
    if (z_view_keyexpr_from_str(&vke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        return -1;
    }
    z_owned_keyexpr_t ke;
    if (z_declare_keyexpr(z_loan(s), &ke, z_loan(vke)) < 0) {
        z_drop(z_move(s));
        return -1;
    }

    // Create payload
    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, value);

    printf("Putting Data ('%s': '%s')...\n", keyexpr, value);
    if (z_put(z_loan(s), z_loan(ke), z_move(payload), NULL) < 0) {
        printf("Oh no! Put has failed...\n");
    }
    // Clean up
    z_undeclare_keyexpr(z_loan(s), z_move(ke));
    z_drop(z_move(s));
    return 0;
}

// Note: All args can be specified multiple times. For "-e" it will append the list of endpoints, for the other it will
// simply replace the previous value.
static int parse_args(int argc, char **argv, z_owned_config_t *config, char **keyexpr, char **value) {
    int opt;
    while ((opt = getopt(argc, argv, "k:v:e:m:l:")) != -1) {
        switch (opt) {
            case 'k':
                *keyexpr = optarg;
                break;
            case 'v':
                *value = optarg;
                break;
            case 'e':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_CONNECT_KEY, optarg);
                break;
            case 'm':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_MODE_KEY, optarg);
                break;
            case 'l':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_LISTEN_KEY, optarg);
                break;
            case '?':
                if (optopt == 'k' || optopt == 'v' || optopt == 'e' || optopt == 'm' || optopt == 'l') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }
    return 0;
}

#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
    return -2;
}
#endif
