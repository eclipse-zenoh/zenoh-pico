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

void query_handler(const z_query_t *query, void *ctx) {
    (void)(ctx);
    z_owned_str_t keystr = z_keyexpr_to_string(z_query_keyexpr(query));
    z_bytes_t pred = z_query_parameters(query);
    z_value_t payload_value = z_query_value(query);
    printf(" >> [Queryable handler] Received Query '%s?%.*s'\n", z_loan(keystr), (int)pred.len, pred.start);
    if (payload_value.payload.len > 0) {
        printf("     with value '%.*s'\n", (int)payload_value.payload.len, payload_value.payload.start);
    }
#if Z_FEATURE_ATTACHMENT == 1
    if (z_attachment_check(&query->_val._rc.in->val.attachment)) {
        printf("Attachement found\n");
        z_attachment_iterate(query->_val._rc.in->val.attachment, attachment_handler, NULL);
    }
    z_attachment_drop(&((z_query_t *)query)->_val._rc.in->val.attachment);
#endif

    z_query_reply_options_t options = z_query_reply_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);

#if Z_FEATURE_ATTACHMENT == 1
    // Add attachment
    z_owned_bytes_map_t map = z_bytes_map_new();
    z_bytes_map_insert_by_alias(&map, _z_bytes_wrap((uint8_t *)"hello", 5), _z_bytes_wrap((uint8_t *)"world", 5));
    options.attachment = z_bytes_map_as_attachment(&map);
#endif

    z_query_reply(query, z_keyexpr(keyexpr), (const unsigned char *)value, strlen(value), &options);
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

    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    if (clocator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, z_string_make(clocator));
    }
    if (llocator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_LISTEN_KEY, z_string_make(llocator));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
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

    printf("Press CTRL-C to quit...\n");
    while (1) {
        if ((n != 0) && (msg_nb >= n)) {
            break;
        }
        sleep(1);
    }

    z_undeclare_queryable(z_move(qable));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));

    z_close(z_move(s));

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERYABLE but this example requires it.\n");
    return -2;
}
#endif
