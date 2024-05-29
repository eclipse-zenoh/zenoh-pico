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

#if Z_FEATURE_QUERYABLE == 1
const char *keyexpr = "demo/example/zenoh-pico-queryable";
const char *value = "Queryable from Pico!";
static int msg_nb = 0;

#if Z_FEATURE_ATTACHMENT == 1
int8_t attachment_handler(z_bytes_t key, z_bytes_t att_value, void *ctx) {
    (void)ctx;
    printf(">>> %.*s: %.*s\n", (int)key.len, key.start, (int)att_value.len, att_value.start);
    return 0;
}
#endif

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
#if Z_FEATURE_ATTACHMENT == 1
    z_attachment_t attachment = z_query_attachment(query);
    if (z_attachment_check(&attachment)) {
        printf("Attachement found\n");
        z_attachment_iterate(attachment, attachment_handler, NULL);
    }
#endif

    z_query_reply_options_t options;
    z_query_reply_options_default(&options);
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);

#if Z_FEATURE_ATTACHMENT == 1
    // Add attachment
    z_owned_bytes_map_t map = z_bytes_map_new();
    z_bytes_map_insert_by_alias(&map, z_bytes_from_str("hello"), z_bytes_from_str("world"));
    options.attachment = z_bytes_map_as_attachment(&map);
#endif

    // Reply value encoding
    z_view_string_t reply_str;
    z_view_str_wrap(&reply_str, value);
    z_owned_bytes_t reply_payload;
    z_bytes_encode_from_string(&reply_payload, z_loan(reply_str));

    z_query_reply(query, z_query_keyexpr(query), z_move(reply_payload), &options);
    z_drop(z_move(keystr));
    msg_nb++;

#if Z_FEATURE_ATTACHMENT == 1
    z_bytes_map_drop(&map);
#endif
}

int main(int argc, char **argv) {
    const char *mode = "client";
    char *clocator = NULL;
    char *llocator = NULL;
    int n = 0;

    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:v:l:n:")) != -1) {
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
            case 'n':
                n = atoi(optarg);
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm' || optopt == 'v' || optopt == 'l' ||
                    optopt == 'n') {
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
        if ((n != 0) && (msg_nb >= n)) {
            break;
        }
        sleep(1);
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
