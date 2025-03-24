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

typedef struct kv_pair_t {
    z_owned_string_t key;
    z_owned_string_t value;
} kv_pair_t;

#if Z_FEATURE_QUERYABLE == 1
char *keyexpr = "demo/example/zenoh-pico-queryable";
char *const default_value = "Queryable from Pico!";
char *value = default_value;
int msg_nb = 0;

static int parse_args(int argc, char **argv, z_owned_config_t *config, char **ke, char **val, int *n);

void print_attachment(const kv_pair_t *kvp, size_t len) {
    printf("    with attachment:\n");
    for (size_t i = 0; i < len; i++) {
        printf("     %zu: %.*s, %.*s\n", i, (int)z_string_len(z_loan(kvp[i].key)), z_string_data(z_loan(kvp[i].key)),
               (int)z_string_len(z_loan(kvp[i].value)), z_string_data(z_loan(kvp[i].value)));
    }
}

void drop_attachment(kv_pair_t *kvp, size_t len) {
    for (size_t i = 0; i < len; i++) {
        z_drop(z_move(kvp[i].key));
        z_drop(z_move(kvp[i].value));
    }
}

void query_handler(z_loaned_query_t *query, void *ctx) {
    (void)(ctx);
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_query_keyexpr(query), &keystr);
    z_view_string_t params;
    z_query_parameters(query, &params);
    printf(" >> [Queryable handler] Received Query '%.*s%.*s'\n", (int)z_string_len(z_loan(keystr)),
           z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(params)), z_string_data(z_loan(params)));
    // Process encoding
    z_owned_string_t encoding;
    z_encoding_to_string(z_query_encoding(query), &encoding);
    printf("    with encoding: %.*s\n", (int)z_string_len(z_loan(encoding)), z_string_data(z_loan(encoding)));

    // Process value
    z_owned_string_t payload_string;
    z_bytes_to_string(z_query_payload(query), &payload_string);
    if (z_string_len(z_loan(payload_string)) > 0) {
        printf("    with value '%.*s'\n", (int)z_string_len(z_loan(payload_string)),
               z_string_data(z_loan(payload_string)));
    }
    z_drop(z_move(payload_string));
    z_drop(z_move(encoding));

    // Check attachment
    const z_loaned_bytes_t *attachment = z_query_attachment(query);
    ze_deserializer_t deserializer = ze_deserializer_from_bytes(attachment);
    size_t attachment_len;
    if (ze_deserializer_deserialize_sequence_length(&deserializer, &attachment_len) == Z_OK) {
        kv_pair_t *kvp = (kv_pair_t *)malloc(sizeof(kv_pair_t) * attachment_len);
        for (size_t i = 0; i < attachment_len; ++i) {
            ze_deserializer_deserialize_string(&deserializer, &kvp[i].key);
            ze_deserializer_deserialize_string(&deserializer, &kvp[i].value);
        }
        if (attachment_len > 0) {
            print_attachment(kvp, attachment_len);
        }
        drop_attachment(kvp, attachment_len);
        free(kvp);
    }

    // Reply payload
    z_owned_bytes_t reply_payload;
    z_bytes_from_static_str(&reply_payload, value);

    z_query_reply_options_t options;
    z_query_reply_options_default(&options);

    // Reply attachment
    z_owned_bytes_t reply_attachment;
    kv_pair_t kvs[1];
    z_string_from_str(&kvs[0].key, "reply_key", NULL, NULL);
    z_string_from_str(&kvs[0].value, "reply_value", NULL, NULL);
    ze_owned_serializer_t serializer;
    ze_serializer_empty(&serializer);
    ze_serializer_serialize_sequence_length(z_loan_mut(serializer), 1);
    ze_serializer_serialize_string(z_loan_mut(serializer), z_loan(kvs[0].key));
    ze_serializer_serialize_string(z_loan_mut(serializer), z_loan(kvs[0].value));
    ze_serializer_finish(z_move(serializer), &reply_attachment);
    options.attachment = z_move(reply_attachment);
    drop_attachment(kvs, 1);

    // Reply encoding
    z_owned_encoding_t reply_encoding;
    z_encoding_from_str(&reply_encoding, "zenoh/string;utf8");
    options.encoding = z_move(reply_encoding);

    z_query_reply(query, z_query_keyexpr(query), z_move(reply_payload), &options);
    msg_nb++;
}

int main(int argc, char **argv) {
    int n = 0;

    z_owned_config_t config;
    z_config_default(&config);
    int ret = parse_args(argc, argv, &config, &keyexpr, &value, &n);
    if (ret != 0) {
        return ret;
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }

    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        return -1;
    }

    printf("Creating Queryable on '%s'...\n", keyexpr);
    z_owned_closure_query_t callback;
    z_closure(&callback, query_handler, NULL, NULL);
    z_owned_queryable_t qable;
    if (z_declare_queryable(z_loan(s), &qable, z_loan(ke), z_move(callback), NULL) < 0) {
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

    z_drop(z_move(qable));

    z_drop(z_move(s));

    return 0;
}

// Note: All args can be specified multiple times. For "-e" it will append the list of endpoints, for the other it will
// simply replace the previous value.
static int parse_args(int argc, char **argv, z_owned_config_t *config, char **ke, char **val, int *n) {
    int opt;
    while ((opt = getopt(argc, argv, "k:v:e:m:l:n:")) != -1) {
        switch (opt) {
            case 'k':
                *ke = optarg;
                break;
            case 'v':
                *val = optarg;
                break;
            case 'e':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_CONNECT_KEY, optarg);
                break;
            case 'm':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_MODE_KEY, optarg);
                break;
            case 'l':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_LISTEN_KEY, optarg);
                break;
            case 'n':
                *n = atoi(optarg);
                break;
            case '?':
                if (optopt == 'k' || optopt == 'v' || optopt == 'e' || optopt == 'm' || optopt == 'l' ||
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
    return 0;
}

#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERYABLE but this example requires it.\n");
    return -2;
}
#endif
