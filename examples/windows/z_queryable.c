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

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <zenoh-pico.h>

#if Z_FEATURE_QUERYABLE == 1
const char *keyexpr = "demo/example/zenoh-pico-queryable";
const char *value = "Queryable from Pico!";

void query_handler(const z_loaned_query_t *query, void *ctx) {
    (void)(ctx);
    z_owned_string_t keystr;
    z_keyexpr_to_string(z_query_keyexpr(query), &keystr);

    z_view_string_t params;
    z_query_parameters(query, &params);
    printf(" >> [Queryable handler] Received Query '%s%.*s'\n", z_str_data(z_loan(keystr)), (int)z_loan(params)->len,
           z_loan(params)->val);
    // Process value
    const z_loaned_bytes_t *payload = z_value_payload(z_query_value(query));
    if (z_bytes_len(payload) > 0) {
        z_owned_string_t payload_string;
        z_bytes_decode_into_string(payload, &payload_string);
        printf("     with value '%s'\n", z_str_data(z_loan(payload_string)));
        z_drop(z_move(payload_string));
    }
    z_query_reply_options_t options;
    z_query_reply_options_default(&options);
    // Reply value encoding
    z_view_string_t reply_str;
    z_view_str_wrap(&reply_str, value);
    z_owned_bytes_t reply_payload;
    z_bytes_encode_from_string(&reply_payload, z_loan(reply_str));

    z_query_reply(query, z_query_keyexpr(query), z_move(reply_payload), &options);

    z_drop(z_move(keystr));
}

int main(int argc, char **argv) {
    (void)(argc);
    (void)(argv);
    char *locator = NULL;

    z_owned_config_t config;
    z_config_default(&config);
    if (locator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, locator);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config)) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
        return -1;
    }

    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_string(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression", keyexpr);
        return -1;
    }

    printf("Creating Queryable on '%s'...\n", keyexpr);
    z_owned_closure_query_t callback;
    z_closure(&callback, query_handler);
    z_owned_queryable_t qable;
    if (z_declare_queryable(&qable, z_loan(s), z_loan(ke), z_move(callback), NULL) < 0) {
        printf("Unable to create queryable.\n");
        return -1;
    }

    printf("Press CTRL-C to quit...\n");
    while (1) {
        Sleep(1);
    }

    z_undeclare_queryable(z_move(qable));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_loan_mut(s));
    zp_stop_lease_task(z_loan_mut(s));

    z_close(z_move(s));

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERYABLE but this example requires it.\n");
    return -2;
}
#endif
