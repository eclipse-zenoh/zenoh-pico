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

void data_handler(const z_sample_t *sample, const void *arg)
{
    (void) (arg);
    printf(">> [Subscriber] Received ('%s': '%.*s')\n",
           sample->key.suffix, (int)sample->value.len, sample->value.start);
}

int main(int argc, char **argv)
{
    z_init_logger();

    z_str_t expr = "/demo/example/**";

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

    z_owned_subscriber_t sub = z_subscribe(z_loan(s), z_expr_new(expr), z_subinfo_default(), data_handler, NULL);
    if (!z_check(sub))
    {
        printf("Unable to declare subscriber.\n");
        exit(-1);
    }

    printf("Enter 'q' to quit...\n");
    char c = 0;
    while (c != 'q')
    {
        c = getchar();
    }

    z_subscriber_close(z_move(sub));

    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s));
    return 0;
}
