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
    const char *key;
    const char *value;
} kv_pair_t;

typedef struct kv_pairs_tx_t {
    const kv_pair_t *data;
    uint32_t len;
    uint32_t current_idx;
} kv_pairs_tx_t;

typedef struct kv_pair_decoded_t {
    z_owned_string_t key;
    z_owned_string_t value;
} kv_pair_decoded_t;

typedef struct kv_pairs_rx_t {
    kv_pair_decoded_t *data;
    uint32_t len;
    uint32_t current_idx;
} kv_pairs_rx_t;

#define KVP_LEN 16

#if Z_FEATURE_QUERYABLE == 1
const char *keyexpr = "demo/example/zenoh-pico-queryable";
const char *value = "Queryable from Pico!";
static int msg_nb = 0;

size_t kv_pairs_size(kv_pairs_tx_t *kvp) {
    size_t ret = 0;
    for (size_t i = 0; i < kvp->len; i++) {
        // Size fields
        ret += 2 * sizeof(uint32_t);
        // Data size
        ret += strlen(kvp->data[i].key) + strlen(kvp->data[i].value);
    }
    return ret;
}

_Bool create_attachment_iter(z_owned_bytes_t *kv_pair, void *context) {
    kv_pairs_tx_t *kvs = (kv_pairs_tx_t *)(context);
    z_owned_bytes_t k, v;
    if (kvs->current_idx >= kvs->len) {
        return false;
    } else {
        z_bytes_serialize_from_str(&k, kvs->data[kvs->current_idx].key);
        z_bytes_serialize_from_str(&v, kvs->data[kvs->current_idx].value);
        z_bytes_serialize_from_pair(kv_pair, z_move(k), z_move(v));
        kvs->current_idx++;
        return true;
    }
}

void parse_attachment(kv_pairs_rx_t *kvp, const z_loaned_bytes_t *attachment) {
    z_owned_bytes_t kv, first, second;
    z_bytes_iterator_t iter = z_bytes_get_iterator(attachment);

    while (kvp->current_idx < kvp->len && z_bytes_iterator_next(&iter, &kv)) {
        z_bytes_deserialize_into_pair(z_loan(kv), &first, &second);
        z_bytes_deserialize_into_string(z_loan(first), &kvp->data[kvp->current_idx].key);
        z_bytes_deserialize_into_string(z_loan(second), &kvp->data[kvp->current_idx].value);
        z_bytes_drop(&first);
        z_bytes_drop(&second);
        z_bytes_drop(&kv);
        kvp->current_idx++;
    }
}

void print_attachment(kv_pairs_rx_t *kvp) {
    printf("    with attachment:\n");
    for (uint32_t i = 0; i < kvp->current_idx; i++) {
        printf("     %d: %s, %s\n", i, z_string_data(z_loan(kvp->data[i].key)),
               z_string_data(z_loan(kvp->data[i].value)));
    }
}

void drop_attachment(kv_pairs_rx_t *kvp) {
    for (size_t i = 0; i < kvp->current_idx; i++) {
        z_string_drop(&kvp->data[i].key);
        z_string_drop(&kvp->data[i].value);
    }
    z_free(kvp->data);
}

void query_handler(const z_loaned_query_t *query, void *ctx) {
    (void)(ctx);
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_query_keyexpr(query), &keystr);
    z_view_string_t params;
    z_query_parameters(query, &params);
    printf(" >> [Queryable handler] Received Query '%s%.*s'\n", z_string_data(z_loan(keystr)), (int)z_loan(params)->len,
           z_loan(params)->val);
    // Process encoding
    z_owned_string_t encoding;
    z_encoding_to_string(z_query_encoding(query), &encoding);
    printf("    with encoding: %s\n", z_string_data(z_loan(encoding)));

    // Process value
    z_owned_string_t payload_string;
    z_bytes_deserialize_into_string(z_query_payload(query), &payload_string);
    if (z_string_len(z_loan(payload_string)) > 1) {
        printf("    with value '%s'\n", z_string_data(z_loan(payload_string)));
    }
    // Check attachment
    kv_pairs_rx_t kvp = {
        .current_idx = 0, .len = KVP_LEN, .data = (kv_pair_decoded_t *)malloc(KVP_LEN * sizeof(kv_pair_decoded_t))};
    parse_attachment(&kvp, z_query_attachment(query));
    if (kvp.current_idx > 0) {
        print_attachment(&kvp);
    }
    drop_attachment(&kvp);
    z_drop(z_move(payload_string));
    z_drop(z_move(encoding));

    // Reply payload
    z_owned_bytes_t reply_payload;
    z_bytes_serialize_from_str(&reply_payload, value);

    z_query_reply_options_t options;
    z_query_reply_options_default(&options);

    // Reply attachment
    kv_pair_t kvs[1];
    kvs[0] = (kv_pair_t){.key = "reply_key", .value = "reply_value"};
    kv_pairs_tx_t kv_ctx = (kv_pairs_tx_t){.data = kvs, .current_idx = 0, .len = 1};
    z_owned_bytes_t attachment;
    z_bytes_serialize_from_iter(&attachment, create_attachment_iter, (void *)&kv_ctx);
    options.attachment = z_move(attachment);

    // Reply encoding
    z_owned_encoding_t reply_encoding;
    z_encoding_from_str(&reply_encoding, "zenoh/string;utf8");
    options.encoding = z_move(reply_encoding);

    z_query_reply(query, z_query_keyexpr(query), z_move(reply_payload), &options);
    msg_nb++;
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
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
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

    z_close(z_move(s));

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERYABLE but this example requires it.\n");
    return -2;
}
#endif
