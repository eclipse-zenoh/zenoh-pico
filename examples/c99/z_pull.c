/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zenoh-pico.h"

void data_handler(const _z_sample_t *sample, const void *arg)
{
    (void)(arg); // Unused argument

    printf(">> [Subscription listener] Received (%zu:%s, %.*s)\n",
           sample->key.id, sample->key.suffix,
           (int)sample->value.len, sample->value.start);
}

int main(int argc, char **argv)
{
    z_init_logger();

    char *expr = "/demo/example/**";

    if (argc > 1)
    {
        expr = argv[1];
    }
    z_owned_config_t config = zp_config_default();
    if (argc > 2)
    {
        zp_config_insert(z_config_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(argv[2]));
    }

    zp_config_insert(z_config_loan(&config), Z_CONFIG_USER_KEY, z_string_make("user"));
    zp_config_insert(z_config_loan(&config), Z_CONFIG_PASSWORD_KEY, z_string_make("password"));

    printf("Openning session...\n");
    z_owned_session_t s = z_open(z_config_move(&config));
    if (!z_session_check(&s))
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    zp_start_read_task(z_session_loan(&s));
    zp_start_lease_task(z_session_loan(&s));

    printf("Creating Subscriber on '%s'...\n", expr);
    z_subscriber_options_t opts = z_subscriber_options_default();
    z_owned_closure_sample_t callback = z_closure(data_handler);
    z_owned_pull_subscriber_t sub = z_declare_pull_subscriber(z_session_loan(&s), z_keyexpr(expr), &callback, &opts);
    if (!z_subscriber_check(&sub))
    {
        printf("Unable to create subscriber.\n");
        exit(-1);
    }

    printf("Press <enter> to pull data...\n");
    char c = 0;
    while (c != 'q')
    {
        c = getchar();
        z_pull(z_pull_subscriber_loan(&sub));
    }

    z_undeclare_pull_subscriber(z_pull_subscriber_move(&sub));

    zp_stop_read_task(z_session_loan(&s));
    zp_stop_lease_task(z_session_loan(&s));
    z_close(z_session_move(&s));
    return 0;
}
