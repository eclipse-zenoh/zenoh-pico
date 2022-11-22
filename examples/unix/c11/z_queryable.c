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
#include <unistd.h>
#include <zenoh-pico.h>

const char *keyexpr = "demo/example/zenoh-pico-queryable";
const char *value = "Queryable from Pico!";

void query_handler(const z_query_t *query, void *ctx) {
    (void)(ctx);
    char *keystr = z_keyexpr_to_string(z_query_keyexpr(query));
    z_bytes_t pred = z_query_parameters(query);
    z_value_t payload_value = z_query_value(query);
    printf(" >> [Queryable handler] Received Query '%s?%.*s'\n", keystr, (int)pred.len, pred.start);
    if (payload_value.payload.len > 0) {
        printf("     with value '%.*s'\n", (int)payload_value.payload.len, payload_value.payload.start);
    }
    z_query_reply_options_t options = z_query_reply_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
    z_query_reply(query, z_keyexpr(keyexpr), (const unsigned char *)value, strlen(value), &options);
    free(keystr);
}

int main(int argc, char **argv) {
    char *locator = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "k:v:e:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'v':
                value = optarg;
                break;
            case 'e':
                locator = optarg;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'v' || optopt == 'e') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }

    z_owned_config_t config = z_config_default();
    if (locator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_PEER_KEY, z_string_make(locator));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        return -1;
    }

    z_keyexpr_t ke = z_keyexpr(keyexpr);
    if (!z_check(ke)) {
        printf("%s is not a valid key expression", keyexpr);
        return -1;
    }

    printf("Creating Queryable on '%s'...\n", keyexpr);
    z_owned_closure_query_t callback = z_closure(query_handler);
    z_owned_queryable_t qable = z_declare_queryable(z_loan(s), ke, z_move(callback), NULL);
    if (!z_check(qable)) {
        printf("Unable to create queryable.\n");
        return -1;
    }

    printf("Enter 'q' to quit...\n");
    char c = '\0';
    while (c != 'q') {
        fflush(stdin);
        scanf("%c", &c);
    }

    z_undeclare_queryable(z_move(qable));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));

    z_close(z_move(s));

    return 0;
}
