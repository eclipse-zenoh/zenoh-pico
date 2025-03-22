//
// Copyright (c) 2025 ZettaScale Technology
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
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "zenoh-pico.h"

typedef struct {
    uint8_t *payload;
    size_t payload_len;
} z_stats_t;

#define TEST_RUN_TIME_S 20

#if Z_FEATURE_QUERYABLE == 1 && Z_FEATURE_MULTI_THREAD == 1

static int parse_args(int argc, char **argv, z_owned_config_t *config, char **ke, size_t *pkt_size);

void query_handler(z_loaned_query_t *query, void *ctx) {
    z_stats_t *stats = (z_stats_t *)ctx;

    // Reply
    z_owned_bytes_t reply_payload;
    z_bytes_from_static_buf(&reply_payload, stats->payload, stats->payload_len);
    z_query_reply(query, z_query_keyexpr(query), z_move(reply_payload), NULL);
    z_drop(z_move(reply_payload));
}

void drop_stats(void *context) {
    z_stats_t *stats = (z_stats_t *)context;
    free(stats->payload);
    free(context);
}

void *stop_task(void *ctx) {
    z_sleep_s(TEST_RUN_TIME_S);
    _Atomic int *stop_flag = (_Atomic int *)ctx;
    atomic_store_explicit(stop_flag, 1, memory_order_relaxed);
    return NULL;
}

int main(int argc, char **argv) {
    char *keyexpr = "lat";
    size_t payload_len = 8;

    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config, &keyexpr, &payload_len);
    if (ret != 0) {
        return ret;
    }

    z_stats_t *context = malloc(sizeof(z_stats_t));
    context->payload_len = payload_len;
    context->payload = (uint8_t *)malloc(context->payload_len);
    memset(context->payload, 1, context->payload_len);

    // Open session
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_drop(z_move(s));
        exit(-1);
    }

    // Declare Queryable
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, keyexpr);
    z_owned_closure_query_t callback;
    z_closure(&callback, query_handler, drop_stats, (void *)context);
    z_owned_queryable_t qable;
    if (z_declare_queryable(z_loan(s), &qable, z_loan(ke), z_move(callback), NULL) < 0) {
        printf("Unable to create queryable.\n");
        return -1;
    }
    _Atomic int stop_flag;
    atomic_store_explicit(&stop_flag, 0, memory_order_relaxed);
    pthread_t task;
    pthread_create(&task, NULL, stop_task, &stop_flag);

    // Listen until stopped
    while (atomic_load_explicit(&stop_flag, memory_order_relaxed) == 0) {
        z_sleep_s(1);
    }
    pthread_join(task, NULL);
    // Clean up
    z_drop(z_move(qable));
    z_drop(z_move(s));
    exit(0);
}

// Note: All args can be specified multiple times. For "-e" it will append the list of endpoints, for the other it will
// simply replace the previous value.
static int parse_args(int argc, char **argv, z_owned_config_t *config, char **ke, size_t *pkt_size) {
    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:l:s:")) != -1) {
        switch (opt) {
            case 'k':
                *ke = optarg;
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
            case 's':
                *pkt_size = (size_t)atoi(optarg);
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm' || optopt == 'l' || optopt == 's') {
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
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
