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
#include <string.h>
#include "zenoh-pico.h"

int main(int argc, char **argv)
{
    char *uri = "/demo/example/zenoh-pico-pub";
    if (argc > 1)
    {
        uri = argv[1];
    }
    char *value = "Pub from pico!";
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

    // Start the receive and the session lease loop for zenoh-pico
    znp_start_read_task(s);
    znp_start_lease_task(s);

    printf("Declaring Resource '%s'", uri);
    unsigned long rid = zn_declare_resource(s, zn_rname(uri));
    printf(" => RId %lu\n", rid);
    zn_reskey_t reskey = zn_rid(rid);

    char buf[256];
    for (int idx = 0; idx < 5; ++idx)
    {
        sleep(1);
        sprintf(buf, "[%4d] %s", idx, value);
        printf("Writing Data ('%lu': '%s')...\n", rid, buf);
        zn_write(s, reskey, (const uint8_t *)buf, strlen(buf));
    }

    znp_stop_read_task(s);
    znp_stop_lease_task(s);
    zn_close(s);

    return 0;
}
