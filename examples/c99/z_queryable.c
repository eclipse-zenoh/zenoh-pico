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
#include <string.h>
#include <unistd.h>

#include "zenoh-pico.h"

char *expr = "demo/example/zenoh-pico-queryable";
char *value = "Queryable from Pico!";

void query_handler(z_query_t *query, void *ctx)
{
    (void) (ctx);
    // char *res = z_keyexpr_to_string(z_query_keyexpr(query));
    const char *res = z_keyexpr_to_string(query->key);
    // z_bytes_t pred = z_query_value_selector(query);
    printf(">> [Queryable ] Received Query '%s:%s'\n", res, query->predicate);
    // z_query_reply(query, z_query_keyexpr(query), (const unsigned char *)value, strlen(value));
    z_query_reply(query, query->key, (const unsigned char *)value, strlen(value));
    // free(res);
}

int main(int argc, char **argv)
{
    z_init_logger();

    if (argc > 1)
        expr = argv[1];

    z_owned_config_t config = zp_config_default();
    if (argc > 2)
        zp_config_insert(z_config_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(argv[2]));

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_config_move(&config));
    if (!z_session_check(&s))
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_session_loan(&s));
    zp_start_lease_task(z_session_loan(&s));

    z_keyexpr_t keyexpr = z_keyexpr(expr);
    if (!z_keyexpr_is_valid(&keyexpr))
    {
        printf("%s is not a valid key expression", expr);
        exit(-1);
    }

    printf("Creating Queryable on '%s'...\n", expr);
    z_owned_closure_query_t callback = z_closure(query_handler);
    z_owned_queryable_t qable = z_declare_queryable(z_session_loan(&s), keyexpr, z_closure_query_move(&callback), NULL);
    if (!z_queryable_check(&qable))
    {
        printf("Unable to create queryable.\n");
        exit(-1);
    }

    printf("Enter 'q' to quit...\n");
    char c = 0;
    while (c != 'q')
        c = getchar();

    z_undeclare_queryable(z_queryable_move(&qable));

    // Stop the receive and the session lease loop for zenoh-pico
    zp_stop_read_task(z_session_loan(&s));
    zp_stop_lease_task(z_session_loan(&s));

    z_close(z_session_move(&s));

    return 0;
}
