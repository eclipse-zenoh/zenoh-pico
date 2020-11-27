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

char *uri = "/demo/example/zenoh-pico-eval";
char *value = "Eval from pico!";

void query_handler(zn_query_t *query, const void *arg)
{
    (void)(arg); // Unused paramater

    z_string_t res = zn_query_res_name(query);
    z_string_t pred = zn_query_predicate(query);
    printf(">> [Query handler] Handling '%.*s?%.*s'\n", (int)res.len, res.val, (int)pred.len, pred.val);
    zn_send_reply(query, uri, (const unsigned char *)value, strlen(value));
}

int main(int argc, char **argv)
{
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

    printf("Declaring Queryable on '%s'...\n", uri);
    zn_queryable_t *qable = zn_declare_queryable(s, zn_rname(uri), ZN_QUERYABLE_EVAL, query_handler, NULL);
    if (qable == 0)
    {
        printf("Unable to declare queryable.\n");
        exit(-1);
    }

    char c = 0;
    while (c != 'q')
    {
        c = fgetc(stdin);
    }

    zn_undeclare_queryable(qable);
    zn_close(s);

    return 0;
}
