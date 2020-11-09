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
#include <unistd.h>
#include <string.h>
#include "zenoh/net.h"

void reply_handler(const zn_source_info_t *info, const zn_sample_t *sample, const void *arg)
{
    (void)(info); // Unused parameter
    (void)(arg);  // Unused parameter

    printf(">> [Reply handler] received (%.*s, %.*s)\n",
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
    if (argc > 2)
    {
        zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(argv[2]));
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

    printf("Sending Query '%s'...\n", uri);
    zn_query(s, zn_rname(uri), "", zn_query_target_default(), zn_query_consolidation_default(), reply_handler, NULL);

    sleep(1);

    zn_close(s);
    return 0;
}
