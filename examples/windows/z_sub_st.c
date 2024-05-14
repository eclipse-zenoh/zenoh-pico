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
#include <zenoh-pico.h>

#define N 2147483647  // max int value by default
int msg_nb = 0;

#if Z_FEATURE_SUBSCRIPTION == 1
void data_handler(const z_sample_t *sample, void *ctx) {
    (void)(ctx);
    z_keyexpr_t keyexpr = z_sample_keyexpr(sample);
    z_bytes_t payload = z_sample_payload(sample);
    z_owned_str_t keystr = z_keyexpr_to_string(keyexpr);
    printf(">> [Subscriber] Received ('%s': '%.*s')\n", z_loan(keystr), (int)payload.len, payload.start);
    z_drop(z_move(keystr));

    msg_nb++;
}

int main(int argc, char **argv) {
    (void)(argc);
    (void)(argv);
    const char *keyexpr = "demo/example/**";
    const char *mode = "client";
    char *locator = NULL;

    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    if (locator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, z_string_make(locator));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return -1;
    }

    z_owned_closure_sample_t callback = z_closure(data_handler);
    printf("Declaring Subscriber on '%s'...\n", keyexpr);
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(s), z_keyexpr(keyexpr), z_move(callback), NULL);
    if (!z_check(sub)) {
        printf("Unable to declare subscriber.\n");
        return -1;
    }

    printf("Press CTRL-C to quit...\n");
    while (msg_nb < N) {
        zp_read(z_loan(s), NULL);
        zp_send_keep_alive(z_loan(s), NULL);
        zp_send_join(z_loan(s), NULL);
    }

    z_undeclare_subscriber(z_move(sub));

    z_close(z_move(s));

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
