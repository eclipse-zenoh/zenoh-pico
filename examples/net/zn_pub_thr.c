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
    if (argc < 2)
    {
        printf("USAGE:\n\tzn_pub_thr <payload-size> [<zenoh-locator>]\n\n");
        exit(-1);
    }
    size_t len = atoi(argv[1]);
    printf("Running throughput test for payload of %zu bytes.\n", len);
    zn_properties_t *config = zn_config_default();
    if (argc > 2)
    {
        zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(argv[2]));
    }

    zn_session_t *s = zn_open(config);
    if (s == 0)
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    // Start the read session session lease loops
    znp_start_read_task(s);
    znp_start_lease_task(s);

    char *data = (char *)malloc(len);
    memset(data, 1, len);

    zn_reskey_t reskey = zn_rid(zn_declare_resource(s, zn_rname("/test/thr")));
    zn_publisher_t *pub = zn_declare_publisher(s, reskey);
    if (pub == 0)
    {
        printf("Unable to declare publisher.\n");
        exit(-1);
    }

    while (1)
    {
        zn_write_ext(s, reskey, (const uint8_t *)data, len, Z_ENCODING_DEFAULT, Z_DATA_KIND_DEFAULT, zn_congestion_control_t_BLOCK);
    }
}
