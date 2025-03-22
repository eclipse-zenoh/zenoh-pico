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
#include <unistd.h>

#include "zenoh-pico.h"
#include "zenoh-pico/protocol/codec.h"

#define ZENOH_MSH_HDR_SIZE 3
#define ZENOH_FRAME_HDR_SIZE 2
#define ZENOH_TCP_MTU 49150
#define ZENOH_UDP_MTU 1450
#define ZENOH_TCP_PEER_MTU 65533
#define TEST_RUN_TIME_S 20

typedef struct {
    unsigned long frequency;
    _Atomic unsigned long count;
    bool *stop_flag;
} z_stats_t;

#if Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_MULTI_THREAD == 1

static int parse_args(int argc, char **argv, z_owned_config_t *config, char **ke, unsigned long *freq, size_t *pkt_size,
                      bool *is_peer, bool *is_udp);

void z_free_with_context(void *ptr, void *context) {
    (void)context;
    z_free(ptr);
}

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

void *measure_task(void *ctx) {
    z_stats_t *stats = (z_stats_t *)ctx;
    unsigned long sleep_dur_us = 1000000 / stats->frequency;
    while (!*stats->stop_flag) {
        z_clock_t start = z_clock_now();
        z_sleep_us(sleep_dur_us);
        unsigned long elapsed = z_clock_elapsed_us(&start);
        // Clear count
        unsigned long count = cas_loop(stats, 0);
        if (count > 0) {
            printf("%.3lf\n", (double)count * 1000000.0 / (double)elapsed);
        }
    }
    return NULL;
}

void *stop_task(void *ctx) {
    z_sleep_s(TEST_RUN_TIME_S);
    bool *stop_flag = (bool *)ctx;
    *stop_flag = true;
    return NULL;
}

int main(int argc, char **argv) {
    char *keyexpr = "thr";
    size_t len = 8;
    bool is_peer = false;
    bool is_udp = false;

    z_stats_t *context = malloc(sizeof(z_stats_t));
    atomic_store_explicit(&context->count, 0, memory_order_relaxed);
    context->frequency = 10;

    // Calculate max mtu batch size
    size_t batch_size = 0;
#if Z_FEATURE_BATCHING == 1
    uint_fast8_t vle_size = _z_zint_len(len);
    uint_fast8_t vle_str = _z_zint_len(strlen(keyexpr));
    size_t msg_size = 0;
    size_t max_mtu_size = 0;
    size_t zenoh_overhead = 0;
    if (is_peer) {
        if ((is_udp)) {
            zenoh_overhead = vle_size + vle_str + strlen(keyexpr) + ZENOH_MSH_HDR_SIZE;
            msg_size = len + zenoh_overhead;
            max_mtu_size = (ZENOH_UDP_MTU - ZENOH_FRAME_HDR_SIZE);
        } else {
            zenoh_overhead = vle_size + ZENOH_MSH_HDR_SIZE;
            msg_size = len + zenoh_overhead;
            max_mtu_size = (ZENOH_TCP_PEER_MTU - ZENOH_FRAME_HDR_SIZE);
        }
        batch_size = max_mtu_size / msg_size;
    }
    if (!is_peer) {
        zenoh_overhead = vle_size + ZENOH_MSH_HDR_SIZE;
        msg_size = len + zenoh_overhead;
        max_mtu_size = (ZENOH_TCP_MTU - ZENOH_FRAME_HDR_SIZE);
        batch_size = max_mtu_size / msg_size;
    }
    if (batch_size < 2) {
        batch_size = 0;
    }
    // double overhead = (double)(ZENOH_FRAME_HDR_SIZE) / (ZENOH_FRAME_HDR_SIZE + (double)(msg_size * batch_size)) +
    //                   (double)(zenoh_overhead);
#endif

    // Set config
    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config, &keyexpr, &context->frequency, &len, &is_peer, &is_udp);
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
    if (!is_peer) {
        if (zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
            printf("Unable to start lease tasks\n");
            z_drop(z_move(s));
            exit(-1);
        }
        if (zp_start_read_task(z_loan_mut(s), NULL) < 0) {
            printf("Unable to start read tasks\n");
            z_drop(z_move(s));
            exit(-1);
        }
    }
    // Timer for sub to prepare
    z_sleep_s(1);
    // Declare publisher
    z_owned_publisher_t pub;
    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        return -1;
    }
    z_publisher_options_t opts;
    z_publisher_options_default(&opts);
    opts.congestion_control = Z_CONGESTION_CONTROL_DROP;
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), &opts) < 0) {
        printf("Unable to declare publisher for key expression!\n");
        exit(-1);
    }

    uint8_t *value = (uint8_t *)z_malloc(len);
    memset(value, 1, len);

    z_owned_bytes_t payload;
    z_bytes_from_buf(&payload, value, len, z_free_with_context, NULL);

    bool stop_flag = false;
    context->stop_flag = &stop_flag;
    pthread_t task;
    pthread_create(&task, NULL, measure_task, (void *)context);
    pthread_t task2;
    pthread_create(&task2, NULL, stop_task, &stop_flag);

    // Send join
    zp_send_join(z_loan(s), NULL);
    // Send packets
    (void)batch_size;
#if Z_FEATURE_BATCHING == 1
    if (batch_size > 0) zp_batch_start(z_loan(s));
#endif
    z_owned_bytes_t p;
    while (!stop_flag) {
        // Clone payload
        z_bytes_clone(&p, z_loan(payload));
        z_publisher_put(z_loan(pub), z_move(p), NULL);
        atomic_fetch_add_explicit(&context->count, 1, memory_order_relaxed);
    }
#if Z_FEATURE_BATCHING == 1
    if (batch_size > 0) zp_batch_stop(z_loan(s));
#endif

    // Clean up
    pthread_join(task, NULL);
    pthread_join(task2, NULL);
    z_drop(z_move(pub));
    z_drop(z_move(s));
    z_drop(z_move(payload));
    z_free(context);
    exit(0);
}

// Note: All args can be specified multiple times. For "-e" it will append the list of endpoints, for the other it will
// simply replace the previous value.
static int parse_args(int argc, char **argv, z_owned_config_t *config, char **ke, unsigned long *freq, size_t *pkt_size,
                      bool *is_peer, bool *is_udp) {
    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:l:f:s:")) != -1) {
        switch (opt) {
            case 'k':
                *ke = optarg;
                break;
            case 'e':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_CONNECT_KEY, optarg);
                if (strncmp(optarg, "udp", 3) == 0) {
                    *is_udp = true;
                }
                break;
            case 'm':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_MODE_KEY, optarg);
                if (strcmp(optarg, "peer") == 0) {
                    *is_peer = true;
                }
                break;
            case 'l':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_LISTEN_KEY, optarg);
                if (strncmp(optarg, "udp", 3) == 0) {
                    *is_udp = true;
                }
                break;
            case 'f':
                *freq = (unsigned long)atoi(optarg);
                break;
            case 's':
                *pkt_size = (size_t)atoi(optarg);
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm' || optopt == 'l' || optopt == 'f' ||
                    optopt == 's') {
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
