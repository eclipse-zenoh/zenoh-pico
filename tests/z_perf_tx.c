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
#include <unistd.h>

#include "zenoh-pico.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#define TEST_DURATION_US 10000000

int send_packets(size_t pkt_len, z_owned_publisher_t *pub, uint8_t *value) {
    z_clock_t test_start = z_clock_now();
    unsigned long elapsed_us = 0;
    while (elapsed_us < TEST_DURATION_US) {
        if (z_publisher_put(z_loan(*pub), (const uint8_t *)value, pkt_len, NULL) != 0) {
            printf("Put failed for pkt len: %lu\n", pkt_len);
            return -1;
        }
        elapsed_us = z_clock_elapsed_us(&test_start);
    }
    return 0;
}

int main(int argc, char **argv) {
    size_t len_array[] = {1048576, 524288, 262144, 131072, 65536, 32768, 16384, 8192, 4096,
                          2048,    1024,   512,    256,    128,   64,    32,    16,   8};  // Biggest value first
    uint8_t *value = (uint8_t *)malloc(len_array[0]);
    memset(value, 1, len_array[0]);
    char *keyexpr = "test/thr";
    const char *mode = "client";
    char *llocator = NULL;
    char *clocator = NULL;

    // Get args
    int opt;
    while ((opt = getopt(argc, argv, "m:l:e:")) != -1) {
        switch (opt) {
            case 'm':
                mode = optarg;
                break;
            case 'l':
                llocator = optarg;
                break;
            case 'e':
                clocator = optarg;
                break;
            default:
                return -1;
        }
    }
    // Set config
    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(mode));
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
        exit(-1);
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        exit(-1);
    }
    // Declare publisher
    z_owned_publisher_t pub = z_declare_publisher(z_loan(s), z_keyexpr(keyexpr), NULL);
    if (!z_check(pub)) {
        printf("Unable to declare publisher for key expression!\n");
        exit(-1);
    }
    // Wait for joins
    if (strcmp(mode, "peer") == 0) {
        printf("Waiting for JOIN messages\n");
        z_sleep_s(3);
    }
    // Send packets
    for (size_t i = 0; i < ARRAY_SIZE(len_array); i++) {
        printf("Start sending pkt len: %lu\n", len_array[i]);
        if (send_packets(len_array[i], &pub, value) != 0) {
            break;
        }
    }
    // Send end packet
    printf("Sending end pkt\n");
    z_publisher_put(z_loan(pub), (const uint8_t *)value, 1, NULL);
    // Clean up
    z_undeclare_publisher(z_move(pub));
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s));
    free(value);
    exit(0);
}
