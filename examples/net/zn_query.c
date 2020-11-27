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
#include "zenoh-pico/net.h"

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
    zn_reply_data_array_t replies = zn_query_collect(s, zn_rname(uri), "", zn_query_target_default(), zn_query_consolidation_default());

    for (unsigned int i = 0; i < replies.len; ++i)
    {
        printf(">> [Reply handler] received (%.*s, %.*s)\n",
               (int)replies.val[i].data.key.len, replies.val[i].data.key.val,
               (int)replies.val[i].data.value.len, replies.val[i].data.value.val);
    }
    zn_reply_data_array_free(replies);

    zn_close(s);
    return 0;
}
