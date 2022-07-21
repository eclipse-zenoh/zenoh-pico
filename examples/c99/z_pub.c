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
#include <stdio.h>
#include <unistd.h>

#include "zenoh-pico.h"

int main(int argc, char **argv)
{
    z_init_logger();

    char *keyexpr = "demo/example/zenoh-pico-pub";
    char *value = "Pub from Pico!";

    if (argc > 1)
        keyexpr = argv[1];

    if (argc > 2)
        value = argv[2];

    z_owned_config_t config = zp_config_default();
    if (argc > 3)
        zp_config_insert(z_config_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(argv[3]));

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_config_move(&config));
    if (!z_session_check(&s))
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_session_loan(&s));
    zp_start_lease_task(z_session_loan(&s));

    printf("Declaring publisher for '%s'...", keyexpr);
    z_owned_publisher_t pub = z_declare_publisher(z_session_loan(&s), z_keyexpr(keyexpr), NULL);
    if (!z_publisher_check(&pub))
    {
        printf("Unable to declare publisher for key expression!\n");
        exit(-1);
    }

    char buf[256];
    for (int idx = 0; 1; ++idx)
    {
        sleep(1);
        sprintf(buf, "[%4d] %s", idx, value);
        printf("Putting Data ('%s': '%s')...\n", keyexpr, buf);
        z_publisher_put(z_publisher_loan(&pub), (const uint8_t *)buf, strlen(buf), NULL);
    }

    z_undeclare_publisher(z_publisher_move(&pub));

    // Stop the receive and the session lease loop for zenoh-pico
    zp_stop_read_task(z_session_loan(&s));
    zp_stop_lease_task(z_session_loan(&s));

    z_close(z_session_move(&s));

    return 0;
}
