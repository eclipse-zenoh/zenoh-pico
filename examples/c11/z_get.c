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
#include <string.h>

#include "zenoh-pico.h"

void reply_handler(z_reply_t *reply, void *arg)
{
    if (reply->tag == Z_REPLY_TAG_DATA)
    {
        (void) (arg);
        printf(">> [Reply] Received ('%s': '%.*s')\n",
            reply->data.sample.key.suffix, (int)reply->data.sample.value.len, reply->data.sample.value.start);
    }
    else
    {
        printf(">> [Reply] Received Final ('%s')\n",
            reply->data.sample.key.suffix);
    }
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
        zp_config_insert(z_loan(config), Z_CONFIG_PEER_KEY, z_string_make(argv[2]));
    }

    zp_config_insert(z_loan(config), Z_CONFIG_USER_KEY, z_string_make("user"));
    zp_config_insert(z_loan(config), Z_CONFIG_PASSWORD_KEY, z_string_make("password"));

    printf("Openning session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s))
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_loan(s));
    zp_start_lease_task(z_loan(s));

    printf("Sending Query '%s'...\n", expr);
    z_owned_closure_reply_t callback = z_closure(reply_handler);
    if (z_get(z_loan(s), z_keyexpr(expr), "", &callback, NULL) == 0)
    {
        printf("Unable to send query.\n");
        exit(-1);
    }

    printf("Enter 'q' to quit...\n");
    char c = 0;
    while (c != 'q')
    {
        c = getchar();
    }

    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s));
    return 0;
}
