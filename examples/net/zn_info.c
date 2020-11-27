/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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
#include "zenoh-pico/net.h"

int main(int argc, char **argv)
{
    zn_properties_t *config = zn_config_default();
    if (argc > 1)
    {
        zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(argv[1]));
    }

    zn_properties_insert(config, ZN_CONFIG_USER_KEY, z_string_make("user"));
    zn_properties_insert(config, ZN_CONFIG_PASSWORD_KEY, z_string_make("password"));

    printf("Openning session...\n");
    zn_session_t *s = zn_open(config);
    if (s == 0)
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    zn_properties_t *ps = zn_info(s);
    z_string_t prop = zn_properties_get(ps, ZN_INFO_PID_KEY);
    printf("info_pid : %.*s\n", (int)prop.len, prop.val);

    prop = zn_properties_get(ps, ZN_INFO_ROUTER_PID_KEY);
    printf("info_router_pid : %.*s\n", (int)prop.len, prop.val);

    zn_properties_free(ps);
    zn_close(s);

    return 0;
}
