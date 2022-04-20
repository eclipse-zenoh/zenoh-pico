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
           sample->key.rid, sample->key.rname,
           (int)sample->value.len, sample->value.val);
}

int main(int argc, char **argv)
{
    z_init_logger();

    z_str_t expr = "/demo/example/**";

    if (argc > 1)
    {
        expr = argv[1];
    }
    z_owned_config_t config = z_config_default();
    if (argc > 2)
    {
        z_config_insert(z_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(argv[2]));
    }

    z_config_insert(z_loan(&config), Z_CONFIG_USER_KEY, z_string_make("user"));
    z_config_insert(z_loan(&config), Z_CONFIG_PASSWORD_KEY, z_string_make("password"));

    printf("Openning session...\n");
    z_owned_session_t s = z_open(z_move(&config));
    if (!z_check(&s))
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    _zp_start_read_task(z_loan(&s));
    _zp_start_lease_task(z_loan(&s));

    printf("Creating Subscriber on '%s'...\n", expr);
    z_subinfo_t subinfo;
    subinfo.reliability = Z_RELIABILITY_RELIABLE;
    subinfo.mode = Z_SUBMODE_PULL;
    subinfo.period = Z_PERIOD_NONE;
    z_owned_subscriber_t sub = z_subscribe(z_loan(&s), z_expr(expr), subinfo, data_handler, NULL);
    if (!z_check(&sub))
    {
        printf("Unable to create subscriber.\n");
        exit(-1);
    }

    printf("Press <enter> to pull data...\n");
    char c = 0;
    while (c != 'q')
    {
        c = getchar();
        z_pull(z_loan(&sub));
    }

    z_subscriber_close(z_move(&sub));

    _zp_stop_read_task(z_loan(&s));
    _zp_stop_lease_task(z_loan(&s));
    z_close(z_move(&s));
    return 0;
}
