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
//

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

typedef struct kv_pair_t {
    z_owned_string_t key;
    z_owned_string_t value;
} kv_pair_t;

typedef struct kv_pairs_t {
    kv_pair_t *data;
    uint32_t len;
    uint32_t current_idx;
} kv_pairs_t;

#define KVP_LEN 16

#if Z_FEATURE_SUBSCRIPTION == 1

static int msg_nb = 0;

void parse_attachment(kv_pairs_t *kvp, const z_loaned_bytes_t *attachment) {
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

void print_attachment(kv_pairs_t *kvp) {
    printf("    with attachment:\n");
    for (uint32_t i = 0; i < kvp->current_idx; i++) {
        printf("     %d: %s, %s\n", i, z_string_data(z_loan(kvp->data[i].key)),
               z_string_data(z_loan(kvp->data[i].value)));
    }
}

void drop_attachment(kv_pairs_t *kvp) {
    for (size_t i = 0; i < kvp->current_idx; i++) {
        z_string_drop(&kvp->data[i].key);
        z_string_drop(&kvp->data[i].value);
    }
    z_free(kvp->data);
}

void data_handler(const z_loaned_sample_t *sample, void *ctx) {
    (void)(ctx);
    z_owned_string_t keystr;
    z_keyexpr_to_string(z_sample_keyexpr(sample), &keystr);
    z_owned_string_t value;
    z_bytes_deserialize_into_string(z_sample_payload(sample), &value);
    z_owned_string_t encoding;
    z_encoding_to_string(z_sample_encoding(sample), &encoding);

    printf(">> [Subscriber] Received ('%s': '%s')\n", z_string_data(z_loan(keystr)), z_string_data(z_loan(value)));
    printf("    with encoding: %s\n", z_string_data(z_loan(encoding)));
    // Check attachment
    kv_pairs_t kvp = {.current_idx = 0, .len = KVP_LEN, .data = (kv_pair_t *)malloc(KVP_LEN * sizeof(kv_pair_t))};
    parse_attachment(&kvp, z_sample_attachment(sample));
    if (kvp.current_idx > 0) {
        print_attachment(&kvp);
    }
    drop_attachment(&kvp);
    z_drop(z_move(keystr));
    z_drop(z_move(value));
    z_drop(z_move(encoding));
    msg_nb++;
}

int main(int argc, char **argv) {
    const char *keyexpr = "demo/example/**";
    const char *mode = "client";
    char *clocator = NULL;
    char *llocator = NULL;
    int n = 0;

    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:l:n:")) != -1) {
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
            case 'n':
                n = atoi(optarg);
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm' || optopt == 'l' || optopt == 'n') {
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

    z_owned_closure_sample_t callback;
    z_closure(&callback, data_handler);
    printf("Declaring Subscriber on '%s'...\n", keyexpr);
    z_owned_subscriber_t sub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, keyexpr);
    if (z_declare_subscriber(&sub, z_loan(s), z_loan(ke), z_move(callback), NULL) < 0) {
        printf("Unable to declare subscriber.\n");
        return -1;
    }

    printf("Press CTRL-C to quit...\n");
    while (1) {
        if ((n != 0) && (msg_nb >= n)) {
            break;
        }
        sleep(1);
    }
    // Clean up
    z_undeclare_subscriber(z_move(sub));
    zp_stop_read_task(z_loan_mut(s));
    zp_stop_lease_task(z_loan_mut(s));
    z_close(z_move(s));
    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
