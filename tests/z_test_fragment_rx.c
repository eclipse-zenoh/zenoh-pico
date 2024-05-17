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
#include <zenoh-pico.h>

#if Z_FEATURE_SUBSCRIPTION == 1
void data_handler(const z_loaned_sample_t *sample, void *ctx) {
    (void)(ctx);
    z_owned_str_t keystr;
    z_keyexpr_to_string(z_sample_keyexpr(sample), &keystr);
    bool is_valid = true;
    const z_loaned_bytes_t *payload = z_sample_payload(sample);
    const uint8_t *data = payload->start;
    for (size_t i = 0; i < payload->len; i++) {
        if (data[i] != (uint8_t)i) {
            is_valid = false;
            break;
        }
    }
    printf("[rx]: Received packet on %s, len: %d, validity: %d\n", z_str_data(z_loan(keystr)),
           (int)z_sample_payload(sample)->len, is_valid);
    z_drop(z_move(keystr));
}

int main(int argc, char **argv) {
    const char *keyexpr = "test/zenoh-pico-fragment";
    const char *mode = NULL;
    char *llocator = NULL;
    char *clocator = NULL;
    (void)argv;

    // Check if peer mode
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
    // Declare subscriber
    z_owned_closure_sample_t callback;
    z_closure(&callback, data_handler);
    z_owned_subscriber_t sub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_string(&ke, keyexpr);
    if (z_declare_subscriber(&sub, z_loan(s), z_loan(ke), z_move(callback), NULL) < 0) {
        printf("Unable to declare subscriber.\n");
        return -1;
    }
    // Wait for termination
    char c = '\0';
    while (c != 'q') {
        fflush(stdin);
        int ret = scanf("%c", &c);
        (void)ret;  // Remove unused result warning
    }
    // Clean up
    z_undeclare_subscriber(z_move(sub));
    zp_stop_read_task(z_loan_mut(s));
    zp_stop_lease_task(z_loan_mut(s));
    z_close(z_move(s));
    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this test requires it.\n");
    return -2;
}
#endif
