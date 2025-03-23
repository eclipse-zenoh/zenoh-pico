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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"

#if Z_FEATURE_PUBLICATION == 1

void z_free_with_context(void *ptr, void *context) {
    (void)context;
    z_free(ptr);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("USAGE:\n\tz_pub_thr <payload-size> [<zenoh-locator>]\n\n");
        exit(-1);
    }
    char *keyexpr = "test/thr";
    size_t len = (size_t)atoi(argv[1]);
    uint8_t *value = (uint8_t *)z_malloc(len);
    memset(value, 1, len);

    // Set config
    z_owned_config_t config;
    z_config_default(&config);
    if (argc > 2) {
        if (zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, argv[2]) < 0) {
            printf("Couldn't insert locator in config: %s\n", argv[2]);
            exit(-1);
        }
    }
    // Open session
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        exit(-1);
    }
    // Declare publisher
    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        return -1;
    }
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) < 0) {
        printf("Unable to declare publisher for key expression!\n");
        exit(-1);
    }

    z_owned_bytes_t payload;
    z_bytes_from_buf(&payload, value, len, z_free_with_context, NULL);
    // Send packets
    while (1) {
        // Clone payload
        z_owned_bytes_t p;
        z_bytes_clone(&p, z_loan(payload));
        z_publisher_put(z_loan(pub), z_move(p), NULL);
    }
    // Clean up
    z_drop(z_move(pub));
    z_drop(z_move(s));
    z_drop(z_move(payload));
    exit(0);
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
    return -2;
}
#endif
