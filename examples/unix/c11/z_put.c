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
int main(int argc, char **argv) {
    const char *keyexpr = "demo/example/zenoh-pico-put";
    const char *value = "Pub from Pico!";
    const char *mode = "client";
    char *clocator = NULL;
    char *llocator = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "k:v:e:m:l:")) != -1) {
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

    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    if (clocator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, z_string_make(clocator));
    }
    if (llocator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_LISTEN_KEY, z_string_make(llocator));
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

    printf("Declaring key expression '%s'...\n", keyexpr);
    z_owned_keyexpr_t ke = z_declare_keyexpr(z_loan(s), z_keyexpr(keyexpr));
    if (!z_check(ke)) {
        printf("Unable to declare key expression!\n");
        zp_stop_read_task(z_loan(s));
        zp_stop_lease_task(z_loan(s));
        z_close(z_move(s));
        return -1;
    }

    printf("Putting Data ('%s': '%s')...\n", keyexpr, value);
    z_put_options_t options = z_put_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
#if Z_FEATURE_ATTACHMENT == 1
    z_owned_bytes_map_t map = z_bytes_map_new();
    z_bytes_map_insert_by_alias(&map, _z_bytes_wrap((uint8_t *)"hi", 2), _z_bytes_wrap((uint8_t *)"there", 5));
    options.attachment = z_bytes_map_as_attachment(&map);
#endif
    if (z_put(z_loan(s), z_keyexpr(keyexpr), (const uint8_t *)value, strlen(value), &options) < 0) {
        printf("Oh no! Put has failed...\n");
    }

#if Z_FEATURE_ATTACHMENT == 1
    z_bytes_map_drop(&map);
#endif

    // Clean up
    z_undeclare_keyexpr(z_loan(s), z_move(ke));
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s));
    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
    return -2;
}
#endif
