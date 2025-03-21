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
#include <unistd.h>

#include "zenoh-pico.h"

#if Z_FEATURE_QUERY == 1 && Z_FEATURE_MULTI_THREAD == 1 && defined Z_FEATURE_UNSTABLE_API

#define DEFAULT_PKT_SIZE 8
#define DEFAULT_PING_NB 100
#define DEFAULT_WARMUP_MS 1000

static int parse_args(int argc, char **argv, z_owned_config_t *config, unsigned int *size, unsigned int *ping_nb,
                      unsigned int *warmup_ms, bool *is_peer);

static _Atomic unsigned long sync_tx_rx = 0;

static unsigned long load_loop(unsigned long target_value) {
    unsigned long curr_val;
    do {
        curr_val = atomic_load_explicit(&sync_tx_rx, memory_order_relaxed);
    } while (curr_val != target_value);
    return curr_val;
}

void reply_handler(z_loaned_reply_t *reply, void *ctx) {
    (void)reply;
    (void)ctx;
    atomic_fetch_add_explicit(&sync_tx_rx, 1, memory_order_relaxed);
}

int main(int argc, char **argv) {
    unsigned int pkt_size = DEFAULT_PKT_SIZE;
    unsigned int ping_nb = DEFAULT_PING_NB;
    unsigned int warmup_ms = DEFAULT_WARMUP_MS;
    bool is_peer = false;

    // Set config
    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config, &pkt_size, &ping_nb, &warmup_ms, &is_peer);
    if (ret != 0) {
        return ret;
    }

    // Open session
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_drop(z_move(s));
        exit(-1);
    }
    if (!is_peer) {
        if (zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
            printf("Unable to start lease tasks\n");
            z_drop(z_move(s));
            return -1;
        }
    }
    // Wait for queryable to stabilize
    z_sleep_ms(1);

    // Get keyexpr & closure
    z_view_keyexpr_t ke;
    z_owned_querier_t que;
    z_view_keyexpr_from_str_unchecked(&ke, "lat");
    if (z_declare_querier(z_loan(s), &que, z_loan(ke), NULL) < 0) {
        printf("Unable to declare querier for key expression!\n");
        return -1;
    }
    unsigned long prev_val = sync_tx_rx;
    z_owned_closure_reply_t callback;

    // Send packets
    if (warmup_ms) {
        z_clock_t warmup_start = z_clock_now();
        unsigned long elapsed_us = 0;
        while (elapsed_us < warmup_ms * 1000) {
            z_closure(&callback, reply_handler, NULL, NULL);
            if (z_querier_get(z_loan(que), NULL, z_move(callback), NULL) != 0) {
                printf("Tx failed");
                continue;
            }
            prev_val = load_loop(prev_val + 1);
            elapsed_us = z_clock_elapsed_us(&warmup_start);
        }
    }

    unsigned long *results = z_malloc(sizeof(unsigned long) * ping_nb);
    for (unsigned int i = 0; i < ping_nb; i++) {
        z_clock_t measure_start = z_clock_now();
        z_closure(&callback, reply_handler, NULL, NULL);
        if (z_querier_get(z_loan(que), NULL, z_move(callback), NULL) != 0) {
            printf("Tx failed");
            continue;
        }
        prev_val = load_loop(prev_val + 1);
        results[i] = z_clock_elapsed_us(&measure_start);
    }
    for (unsigned int i = 0; i < ping_nb; i++) {
        printf("%lu\n", results[i]);
    }
    // Clean up
    z_free(results);
    z_drop(z_move(que));
    z_drop(z_move(s));
    exit(0);
}

// Note: All args can be specified multiple times. For "-e" it will append the list of endpoints, for the other it will
// simply replace the previous value.
static int parse_args(int argc, char **argv, z_owned_config_t *config, unsigned int *size, unsigned int *ping_nb,
                      unsigned int *warmup_ms, bool *is_peer) {
    int opt;
    while ((opt = getopt(argc, argv, "s:n:w:e:m:l:")) != -1) {
        switch (opt) {
            case 's':
                *size = (unsigned int)atoi(optarg);
                break;
            case 'n':
                *ping_nb = (unsigned int)atoi(optarg);
                break;
            case 'w':
                *warmup_ms = (unsigned int)atoi(optarg);
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
            case '?':
                if (optopt == 's' || optopt == 'n' || optopt == 'w' || optopt == 'e' || optopt == 'm' ||
                    optopt == 'l') {
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
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
    return -2;
}
#endif
