//
// Copyright (c) 2024 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include <stdio.h>
#include <zenoh-pico.h>

#include "config.h"

#if Z_FEATURE_QUERYABLE == 1

#define KEYEXPR "demo/example/zenoh-pico-queryable"
#define VALUE "[RPI] Queryable from Zenoh-Pico!"

void query_handler(z_loaned_query_t *query, void *ctx) {
    (void)(ctx);
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_query_keyexpr(query), &keystr);
    z_view_string_t params;
    z_query_parameters(query, &params);
    printf(" >> [Queryable handler] Received Query '%.*s%.*s'\n", (int)z_string_len(z_loan(keystr)),
           z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(params)), z_string_data(z_loan(params)));
    // Process value
    z_owned_string_t payload_string;
    z_bytes_to_string(z_query_payload(query), &payload_string);
    if (z_string_len(z_loan(payload_string)) > 0) {
        printf("     with value '%.*s'\n", (int)z_string_len(z_loan(payload_string)),
               z_string_data(z_loan(payload_string)));
    }
    z_drop(z_move(payload_string));

    z_query_reply_options_t options;
    z_query_reply_options_default(&options);
    // Reply value encoding
    z_owned_bytes_t reply_payload;
    z_bytes_from_static_str(&reply_payload, VALUE);

    z_query_reply(query, z_query_keyexpr(query), z_move(reply_payload), &options);
}

void app_main(void) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, ZENOH_CONFIG_MODE);
    if (strcmp(ZENOH_CONFIG_CONNECT, "") != 0) {
        printf("Connect endpoint: %s\n", ZENOH_CONFIG_CONNECT);
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, ZENOH_CONFIG_CONNECT);
    }
    if (strcmp(ZENOH_CONFIG_LISTEN, "") != 0) {
        printf("Listen endpoint: %s\n", ZENOH_CONFIG_LISTEN);
        zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, ZENOH_CONFIG_LISTEN);
    }

    printf("Opening %s session ...\n", ZENOH_CONFIG_MODE);
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_drop(z_move(s));
        return;
    }

    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, KEYEXPR) < 0) {
        printf("%s is not a valid key expression\n", KEYEXPR);
        return;
    }

    printf("Creating Queryable on '%s'...\n", KEYEXPR);
    z_owned_closure_query_t callback;
    z_closure(&callback, query_handler, NULL, NULL);
    z_owned_queryable_t qable;
    if (z_declare_queryable(z_loan(s), &qable, z_loan(ke), z_move(callback), NULL) < 0) {
        printf("Unable to create queryable.\n");
        return;
    }

    while (1) {
        z_sleep_s(1);
    }

    z_drop(z_move(qable));

    z_drop(z_move(s));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERYABLE but this example requires it.\n");
}
#endif
