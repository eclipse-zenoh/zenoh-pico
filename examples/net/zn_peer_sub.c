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

void data_handler(const zn_sample_t *sample, const void *arg)
{
    (void)(arg); // Unused argument

    printf(">> [Subscription listener] Received (%.*s, %.*s)\n",
           (int)sample->key.len, sample->key.val,
           (int)sample->value.len, sample->value.val);
}

int main(int argc, char **argv)
{
    char *uri = "/demo/example/**";
    if (argc > 1)
    {
        uri = argv[1];
    }

    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_MODE_KEY, z_string_make("peer"));
    if (argc > 2)
        zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(argv[2]));
    else
        zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make("udp/224.0.0.225:7447#iface=en0"));

    printf("Openning session...\n");
    zn_session_t *s = zn_open(config);
    if (s == 0)
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    // Start the read session session lease loops
    znp_start_read_task(s);
    znp_start_lease_task(s);

    printf("Declaring Subscriber on '%s'...\n", uri);
    zn_subscriber_t *sub = zn_declare_subscriber(s, zn_rname(uri), zn_subinfo_default(), data_handler, NULL);
    if (sub == 0)
    {
        printf("Unable to declare subscriber.\n");
        exit(-1);
    }

    char c = 0;
    while (c != 'q')
    {
        c = fgetc(stdin);
    }

    zn_undeclare_subscriber(sub);

    znp_stop_read_task(s);
    znp_stop_lease_task(s);
    zn_close(s);

    return 0;
}
