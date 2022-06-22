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

    char *expr = "/demo/example/zenoh-pico-pub";
    char *value = "Pub from Pico!";

    z_owned_config_t config = z_config_default();
    if (argc > 1)
    {
        z_config_insert(z_loan(config), Z_CONFIG_PEER_KEY, z_string_make(argv[1]));
    }

    z_config_insert(z_loan(config), Z_CONFIG_USER_KEY, z_string_make("user"));
    z_config_insert(z_loan(config), Z_CONFIG_PASSWORD_KEY, z_string_make("password"));

    printf("Openning session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s))
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    zp_start_read_task(z_loan(s));
    zp_start_lease_task(z_loan(s));

    printf("Declaring key expression '%s'...\n", expr);
    z_owned_keyexpr_t keyexpr = z_declare_keyexpr(z_loan(s), z_keyexpr(expr));
    z_publisher_options_t options = z_publisher_options_default();
    z_owned_publisher_t pub = z_declare_publisher(z_loan(s), *z_loan(keyexpr), &options);
    if (!z_check(pub))
    {
        printf("Unable to declare publication.\n");
        goto EXIT;
    }

    char buf[256];
    for (int idx = 0; idx < 5; ++idx)
    {
        sleep(1);
        sprintf(buf, "[%4d] %s", idx, value);
        printf("Putting Data ('%zu': '%s')...\n", z_loan(keyexpr)->id, buf);
        z_put(z_loan(s), z_loan(keyexpr), (const uint8_t *)buf, strlen(buf), NULL);
    }

EXIT:
    z_publisher_delete(z_move(pub));
    z_undeclare_expr(z_loan(s), z_move(keyexpr));

    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s));
    return 0;
}
