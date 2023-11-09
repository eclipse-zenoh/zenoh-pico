//
// Copyright (c) 2022 ZettaScale Technology
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
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <zenoh-pico.h>

#if Z_FEATURE_QUERYABLE == 1
#define CLIENT_OR_PEER 0  // 0: Client mode; 1: Peer mode
#if CLIENT_OR_PEER == 0
#define MODE "client"
#define CONNECT ""  // If empty, it will scout
#elif CLIENT_OR_PEER == 1
#define MODE "peer"
#define CONNECT "udp/224.0.0.225:7447"
#else
#error "Unknown Zenoh operation mode. Check CLIENT_OR_PEER value."
#endif

#define KEYEXPR "demo/example/zenoh-pico-queryable"
#define VALUE "[FreeRTOS-Plus-TCP] Queryable from Zenoh-Pico!"

void query_handler(const z_query_t *query, void *ctx) {
    (void)(ctx);
    z_owned_str_t keystr = z_keyexpr_to_string(z_query_keyexpr(query));
    z_bytes_t pred = z_query_parameters(query);
    z_value_t payload_value = z_query_value(query);
    printf(" >> [Queryable handler] Received Query '%s?%.*s'\n", z_loan(keystr), (int)pred.len, pred.start);
    if (payload_value.payload.len > 0) {
        printf("     with value '%.*s'\n", (int)payload_value.payload.len, payload_value.payload.start);
    }
    z_query_reply_options_t options = z_query_reply_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
    z_query_reply(query, z_keyexpr(KEYEXPR), (const unsigned char *)VALUE, strlen(VALUE), &options);
    z_drop(z_move(keystr));
}

void app_main(void) {
    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(MODE));
    if (strcmp(CONNECT, "") != 0) {
        zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, z_string_make(CONNECT));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        return;
    }

    z_keyexpr_t ke = z_keyexpr(KEYEXPR);
    if (!z_check(ke)) {
        printf("%s is not a valid key expression\n", KEYEXPR);
        return;
    }

    printf("Creating Queryable on '%s'...\n", KEYEXPR);
    z_owned_closure_query_t callback = z_closure(query_handler);
    z_owned_queryable_t qable = z_declare_queryable(z_loan(s), ke, z_move(callback), NULL);
    if (!z_check(qable)) {
        printf("Unable to create queryable.\n");
        return;
    }

    while (1) {
        z_sleep_s(5);
    }

    z_undeclare_queryable(z_move(qable));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));

    z_close(z_move(s));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERYABLE but this example requires it.\n");
}
#endif
