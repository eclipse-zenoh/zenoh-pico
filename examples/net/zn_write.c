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
#include <string.h>
#include "zenoh-pico/net.h"

int main(int argc, char **argv)
{
    // char *uri = "/demo/example/zenoh-pico-write";
    char *uri = "/demo/example/0";

    if (argc > 1)
    {
        uri = argv[1];
    }
    char *value = "Write from pico!";
    if (argc > 2)
    {
        value = argv[2];
    }
    zn_properties_t *config = zn_config_default();
    if (argc > 3)
    {
        zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(argv[3]));
    }

    printf("Openning session...\n");
    zn_session_t *s = zn_open(config);
    if (s == 0)
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    printf("Writing Data ('%s': '%s')...\n", uri, value);
    zn_write(s, zn_rname(uri), (const uint8_t *)value, strlen(value));

    zn_close(s);

    return 0;
}
