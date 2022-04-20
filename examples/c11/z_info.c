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
#include "zenoh-pico.h"

int main(int argc, char **argv)
{
    z_init_logger();

    z_owned_config_t config = z_config_default();
    if (argc > 1)
    {
        z_config_insert(z_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(argv[1]));
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

    z_owned_info_t ops = z_info(z_loan(&s));
    z_info_t ps = z_loan(&ops);

    z_str_t prop = z_info_get(ps, Z_INFO_PID_KEY);
    printf("info_pid : %s\n", prop);

    prop = z_info_get(ps, Z_INFO_ROUTER_PID_KEY);
    printf("info_router_pid : %s\n", prop);

    prop = z_info_get(ps, Z_INFO_PEER_PID_KEY);
    printf("info_peer_pid : %s\n", prop);

    z_info_clear(z_move(&ops));
    z_close(z_move(&s));
    return 0;
}
