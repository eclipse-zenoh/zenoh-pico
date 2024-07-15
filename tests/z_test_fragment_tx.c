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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"

#if Z_FEATURE_PUBLICATION == 1
int main(int argc, char **argv) {
    const char *keyexpr = "test/zenoh-pico-fragment";
    const char *mode = NULL;
    char *llocator = NULL;
    char *clocator = NULL;
    uint8_t *value = NULL;
    size_t size = 10000;
    (void)argv;

    // Init value
    value = malloc(size);
    if (value == NULL) {
        return -1;
    }
    for (size_t i = 0; i < size; i++) {
        value[i] = (uint8_t)i;
    }
    // Check if peer or client mode
    if (argc > 1) {
        mode = "peer";
        llocator = "udp/224.0.0.224:7447#iface=lo";
    } else {
        mode = "client";
        clocator = "tcp/127.0.0.1:7447";
    }
    // Set config
    z_owned_config_t config;
    z_config_default(&config);
    if (mode != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, mode);
    }
    if (llocator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, llocator);
    }
    if (clocator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, clocator);
    }
    // Open session
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
    // Wait for joins
    if (strcmp(mode, "peer") == 0) {
        z_sleep_s(3);
    }
    // Put data
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, keyexpr);
    for (int i = 0; i < 5; i++) {
        // Create payload
        z_owned_bytes_t payload;
        z_bytes_serialize_from_slice(&payload, value, size);

        printf("[tx]: Sending packet on %s, len: %d\n", keyexpr, (int)size);
        if (z_put(z_loan(s), z_loan(ke), z_move(payload), NULL) < 0) {
            printf("Oh no! Put has failed...\n");
            return -1;
        }
        z_sleep_s(1);
    }
    // Clean up
    z_close(z_move(s));
    free(value);
    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this test requires it.\n");
    return -2;
}
#endif
