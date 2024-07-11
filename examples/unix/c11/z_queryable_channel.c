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

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_QUERYABLE == 1
const char *keyexpr = "demo/example/zenoh-pico-queryable";
const char *value = "Queryable from Pico!";

int main(int argc, char **argv) {
    const char *mode = "client";
    char *clocator = NULL;
    char *llocator = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:v:l:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'e':
                clocator = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case 'l':
                llocator = optarg;
                break;
            case 'v':
                value = optarg;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm' || optopt == 'v' || optopt == 'l') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }

    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, mode);
    if (clocator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, clocator);
    }
    if (llocator != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, llocator);
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
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression", keyexpr);
        return -1;
    }

    printf("Creating Queryable on '%s'...\n", keyexpr);
    z_owned_closure_query_t closure;
    z_owned_ring_handler_query_t handler;
    z_ring_channel_query_new(&closure, &handler, 10);
    z_owned_queryable_t qable;
    if (z_declare_queryable(&qable, z_loan(s), z_loan(ke), z_move(closure), NULL) < 0) {
        printf("Unable to create queryable.\n");
        return -1;
    }

    z_owned_query_t query;
    z_null(&query);
    for (z_recv(z_loan(handler), &query); z_check(query); z_recv(z_loan(handler), &query)) {
        const z_loaned_query_t *q = z_loan(query);
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_query_keyexpr(q), &keystr);
        z_view_string_t params;
        z_query_parameters(q, &params);
        printf(" >> [Queryable handler] Received Query '%s%.*s'\n", z_string_data(z_loan(keystr)),
               (int)z_loan(params)->len, z_loan(params)->val);
        // Process value
        z_owned_string_t payload_string;
        z_bytes_deserialize_into_string(z_query_payload(z_loan(query)), &payload_string);
        if (z_string_len(z_loan(payload_string)) > 1) {
            printf("     with value '%s'\n", z_string_data(z_loan(payload_string)));
        }
        z_drop(z_move(payload_string));

        z_query_reply_options_t options;
        z_query_reply_options_default(&options);
        // Reply value encoding
        z_owned_bytes_t reply_payload;
        z_bytes_serialize_from_str(&reply_payload, value);

        z_query_reply(q, z_query_keyexpr(q), z_move(reply_payload), &options);
        z_drop(z_move(query));
    }

    z_drop(z_move(handler));
    z_undeclare_queryable(z_move(qable));

    z_close(z_move(s));

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERYABLE but this example requires it.\n");
    return -2;
}
#endif
