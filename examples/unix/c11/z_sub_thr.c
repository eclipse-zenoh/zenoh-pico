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
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "zenoh-pico.h"

#define TEST_RUN_TIME_S 23

typedef struct {
    _Atomic unsigned long count;
    size_t pkt_len;
} z_stats_t;

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_MULTI_THREAD == 1

static int parse_args(int argc, char **argv, z_owned_config_t *config, char **ke, unsigned long *freq, bool *is_peer);

void on_sample(z_loaned_sample_t *sample, void *context) {
    z_stats_t *stats = (z_stats_t *)context;
    stats->pkt_len = z_bytes_len(z_sample_payload(sample));
    atomic_fetch_add_explicit(&stats->count, 1, memory_order_relaxed);
}

void drop_stats(void *context) { free(context); }

unsigned long cas_loop(z_stats_t *ctx, unsigned long value) {
    unsigned long prev_val = atomic_load_explicit(&ctx->count, memory_order_relaxed);
    unsigned long curr_val = prev_val;
    while (curr_val != value) {
        prev_val = curr_val;
        if (atomic_compare_exchange_weak_explicit(&ctx->count, &curr_val, value, memory_order_acquire,
                                                  memory_order_relaxed)) {
            return prev_val;
        }
    }
    return prev_val;
}

void *stop_task(void *ctx) {
    z_sleep_s(TEST_RUN_TIME_S);
    bool *stop_flag = (bool *)ctx;
    *stop_flag = true;
    return NULL;
}

int main(int argc, char **argv) {
    char *keyexpr = "thr";
    unsigned long frequency = 10;
    bool is_peer = false;

    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config, &keyexpr, &frequency, &is_peer);
    if (ret != 0) {
        return ret;
    }

    z_stats_t *context = malloc(sizeof(z_stats_t));
    atomic_store_explicit(&context->count, 0, memory_order_relaxed);
    context->pkt_len = 0;

    // Open session
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read tasks\n");
        z_drop(z_move(s));
        exit(-1);
    }
    if (!is_peer) {
        if (zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
            printf("Unable to start lease tasks\n");
            z_drop(z_move(s));
            exit(-1);
        }
    }

    // Declare Subscriber/resource
    z_owned_closure_sample_t callback;
    z_closure(&callback, on_sample, drop_stats, (void *)context);
    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        return -1;
    }
    if (z_declare_background_subscriber(z_loan(s), z_loan(ke), z_move(callback), NULL) < 0) {
        printf("Unable to create subscriber.\n");
        exit(-1);
    }
    bool stop_flag = false;
    size_t prev_len = 0;
    pthread_t task;
    pthread_create(&task, NULL, stop_task, &stop_flag);
    unsigned long sleep_dur_us = 1000000 / frequency;
    // Listen until stopped
    while (!stop_flag) {
        z_clock_t start = z_clock_now();
        z_sleep_us(sleep_dur_us);
        unsigned long elapsed = z_clock_elapsed_us(&start);
        // Clear count
        unsigned long count = cas_loop(context, 0);
        if (prev_len != context->pkt_len) {
            prev_len = context->pkt_len;
        }
        if (count > 0) {
            printf("%.3lf\n", (double)count * 1000000.0 / (double)elapsed);
        }
    }
    // Clean up
    z_drop(z_move(s));
    exit(0);
}

// Note: All args can be specified multiple times. For "-e" it will append the list of endpoints, for the other it will
// simply replace the previous value.
static int parse_args(int argc, char **argv, z_owned_config_t *config, char **ke, unsigned long *freq, bool *is_peer) {
    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:l:f:")) != -1) {
        switch (opt) {
            case 'k':
                *ke = optarg;
                break;
            case 'e':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_CONNECT_KEY, optarg);
                break;
            case 'm':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_MODE_KEY, optarg);
                if (strcmp(optarg, "peer") == 0) {
                    *is_peer = true;
                }
                break;
            case 'l':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_LISTEN_KEY, optarg);
                break;
            case 'f':
                *freq = (unsigned long)atoi(optarg);
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm' || optopt == 'l' || optopt == 'f') {
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
