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
#include <zenoh-pico/api/serialization.h>

typedef struct kv_pair_t {
    z_owned_string_t key;
    z_owned_string_t value;
} kv_pair_t;

#if Z_FEATURE_QUERY == 1 && Z_FEATURE_MULTI_THREAD == 1
static z_owned_condvar_t cond;
static z_owned_mutex_t mutex;

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

void reply_dropper(void *ctx) {
    (void)(ctx);
    printf(">> Received query final notification\n");
    z_condvar_signal(z_loan_mut(cond));
    z_drop(z_move(cond));
}

void reply_handler(z_loaned_reply_t *reply, void *ctx) {
    (void)(ctx);
    if (z_reply_is_ok(reply)) {
        const z_loaned_sample_t *sample = z_reply_ok(reply);
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t replystr;
        z_bytes_into_string(z_sample_payload(sample), &replystr);
        z_owned_string_t encoding;
        z_encoding_to_string(z_sample_encoding(sample), &encoding);

        printf(">> Received ('%.*s': '%.*s')\n", (int)z_string_len(z_loan(keystr)), z_string_data(z_loan(keystr)),
               (int)z_string_len(z_loan(replystr)), z_string_data(z_loan(replystr)));
        printf("    with encoding: %.*s\n", (int)z_string_len(z_loan(encoding)), z_string_data(z_loan(encoding)));
        z_drop(z_move(replystr));
        z_drop(z_move(encoding));

        // Check attachment
        const z_loaned_bytes_t *attachment = z_sample_attachment(sample);
        if (attachment == NULL) {
            return;
        }
        ze_deserializer_t deserializer = ze_deserializer(attachment);
        size_t attachment_len;
        ze_deserializer_deserialize_sequence_begin(&deserializer, &attachment_len);
        kv_pair_t *kvp = (kv_pair_t *)malloc(sizeof(kv_pair_t) * attachment_len);
        for (size_t i = 0; i < attachment_len; ++i) {
            ze_deserializer_deserialize_string(&deserializer, &kvp[i].key);
            ze_deserializer_deserialize_string(&deserializer, &kvp[i].value);
        }
        ze_deserializer_deserialize_sequence_end(&deserializer);
        if (attachment_len > 0) {
            print_attachment(kvp, attachment_len);
        }
        drop_attachment(kvp, attachment_len);
        free(kvp);
    } else {
        printf(">> Received an error\n");
    }
}

int main(int argc, char **argv) {
    const char *keyexpr = "demo/example/**";
    const char *mode = "client";
    const char *clocator = NULL;
    const char *llocator = NULL;
    const char *value = NULL;

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

    z_mutex_init(&mutex);
    z_condvar_init(&cond);

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
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s), NULL);
        return -1;
    }

    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression", keyexpr);
        return -1;
    }

    z_mutex_lock(z_loan_mut(mutex));
    printf("Sending Query '%s'...\n", keyexpr);
    z_get_options_t opts;
    z_get_options_default(&opts);
    // Value encoding
    z_owned_bytes_t payload;
    if (value != NULL) {
        z_bytes_from_static_str(&payload, value);
        opts.payload = z_bytes_move(&payload);
    }

    // Add attachment value
    kv_pair_t kvs[1];
    z_string_from_str(&kvs[0].key, "test_key", NULL, NULL);
    z_string_from_str(&kvs[0].value, "test_value", NULL, NULL);
    z_owned_bytes_t attachment;
    z_bytes_empty(&attachment);
    ze_serializer_t serializer = ze_serializer(z_loan_mut(attachment));
    ze_serializer_serialize_sequence_begin(&serializer, 2);
    for (size_t i = 0; i < 1; ++i) {
        ze_serializer_serialize_string(&serializer, z_loan(kvs[i].key));
        ze_serializer_serialize_string(&serializer, z_loan(kvs[i].value));
    }
    ze_serializer_serialize_sequence_end(&serializer);
    drop_attachment(kvs, 1);
    opts.attachment = z_move(attachment);

    // Add encoding value
    z_owned_encoding_t encoding;
    z_encoding_from_str(&encoding, "zenoh/string;utf8");
    opts.encoding = z_move(encoding);

    z_owned_closure_reply_t callback;
    z_closure(&callback, reply_handler, reply_dropper);
    if (z_get(z_loan(s), z_loan(ke), "", z_move(callback), &opts) < 0) {
        printf("Unable to send query.\n");
        return -1;
    }
    z_condvar_wait(z_loan_mut(cond), z_loan_mut(mutex));
    z_mutex_unlock(z_loan_mut(mutex));

    z_close(z_move(s), NULL);
    return 0;
}
#else
int main(void) {
    printf(
        "ERROR: Zenoh pico was compiled without Z_FEATURE_QUERY or Z_FEATURE_MULTI_THREAD but this example requires "
        "them.\n");
    return -2;
}
#endif
