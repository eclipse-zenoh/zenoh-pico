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
    z_owned_config_t config = z_config_default();
    if (mode != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    }
    if (llocator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_LISTEN_KEY, z_string_make(llocator));
    }
    if (clocator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, z_string_make(clocator));
    }
    // Open session
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
    // Wait for joins
    if (strcmp(mode, "peer") == 0) {
        z_sleep_s(3);
    }
    // Put data
    for (int i = 0; i < 5; i++) {
        printf("[tx]: Sending packet on %s, len: %d\n", keyexpr, (int)size);
        if (z_put(z_loan(s), z_keyexpr(keyexpr), (const uint8_t *)value, size, NULL) < 0) {
            printf("Oh no! Put has failed...\n");
        }
        z_sleep_s(1);
    }
    // Clean up
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
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
