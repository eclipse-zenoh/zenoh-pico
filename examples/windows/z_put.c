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

#if Z_FEATURE_PUBLICATION == 1
int main(int argc, char **argv) {
    (void)(argc);
    (void)(argv);
    const char *keyexpr = "demo/example/zenoh-pico-put";
    const char *value = "Pub from Pico!";
    const char *mode = "client";
    char *locator = NULL;

    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, mode);
    if (locator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, locator);
    }

    printf("Opening session...\n");
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

    printf("Declaring key expression '%s'...\n", keyexpr);
    z_view_keyexpr_t vke;
    z_view_keyexpr_from_str(&vke, keyexpr);
    z_owned_keyexpr_t ke;
    if (z_declare_keyexpr(&ke, z_loan(s), z_loan(vke)) < 0) {
        printf("Unable to declare key expression!\n");
        z_close(z_move(s));
        return -1;
    }

    // Create payload
    z_owned_bytes_t payload;
    z_bytes_serialize_from_str(&payload, value);

    printf("Putting Data ('%s': '%s')...\n", keyexpr, value);
    if (z_put(z_loan(s), z_loan(ke), z_move(payload), NULL) < 0) {
        printf("Oh no! Put has failed...\n");
    }

    // Clean up
    z_undeclare_keyexpr(z_move(ke), z_loan(s));
    z_close(z_move(s));
    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
    return -2;
}
#endif
