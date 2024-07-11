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

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <zenoh-pico.h>

void print_zid(const z_id_t *id, void *ctx) {
    (void)(ctx);
    printf(" ");
    for (int i = 15; i >= 0; i--) {
        printf("%02X", id->id[i]);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    (void)(argc);
    (void)(argv);
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

    z_id_t self_id = z_info_zid(z_loan(s));
    printf("Own ID:");
    print_zid(&self_id, NULL);

    printf("Routers IDs:\n");
    z_owned_closure_zid_t callback;
    z_closure(&callback, print_zid);
    z_info_routers_zid(z_loan(s), z_move(callback));

    // `callback` has been `z_move`d just above, so it's safe to reuse the variable,
    // we'll just have to make sure we `z_move` it again to avoid mem-leaks.
    printf("Peers IDs:\n");
    z_owned_closure_zid_t callback2;
    z_closure(&callback2, print_zid);
    z_info_peers_zid(z_loan(s), z_move(callback2));

    z_close(z_move(s));
}
