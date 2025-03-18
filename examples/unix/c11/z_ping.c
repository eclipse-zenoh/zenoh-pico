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
#include "zenoh-pico/system/platform.h"

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_MULTI_THREAD == 1

#define DEFAULT_PKT_SIZE 8
#define DEFAULT_PING_NB 100
#define DEFAULT_WARMUP_MS 1000

static int parse_args(int argc, char** argv, z_owned_config_t* config, unsigned int* size, unsigned int* ping_nb,
                      unsigned int* warmup_ms, bool* is_peer);

static _Atomic unsigned long sync_tx_rx = 0;

void callback(z_loaned_sample_t* sample, void* context) {
    (void)sample;
    (void)context;
    atomic_fetch_add_explicit(&sync_tx_rx, 1, memory_order_relaxed);
}

static unsigned long load_loop(unsigned long target_value) {
    unsigned long curr_val;
    do {
        curr_val = atomic_load_explicit(&sync_tx_rx, memory_order_relaxed);
    } while (curr_val != target_value);
    return curr_val;
}

int main(int argc, char** argv) {
    unsigned int pkt_size = DEFAULT_PKT_SIZE;
    unsigned int ping_nb = DEFAULT_PING_NB;
    unsigned int warmup_ms = DEFAULT_WARMUP_MS;
    bool is_peer = false;

    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config, &pkt_size, &ping_nb, &warmup_ms, &is_peer);
    if (ret != 0) {
        return ret;
    }

    z_owned_session_t session;
    if (z_open(&session, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }
    if (zp_start_read_task(z_loan_mut(session), NULL) < 0) {
        printf("Unable to start read tasks\n");
        z_drop(z_move(session));
        return -1;
    }
    if (!is_peer) {
        if (zp_start_lease_task(z_loan_mut(session), NULL) < 0) {
            printf("Unable to start lease tasks\n");
            z_drop(z_move(session));
            return -1;
        }
    }

    z_view_keyexpr_t ping;
    z_view_keyexpr_from_str_unchecked(&ping, "test/ping");
    z_view_keyexpr_t pong;
    z_view_keyexpr_from_str_unchecked(&pong, "test/pong");

    z_owned_publisher_t pub;
    if (z_declare_publisher(z_loan(session), &pub, z_loan(ping), NULL) < 0) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }

    z_owned_closure_sample_t respond;
    z_closure(&respond, callback);
    if (z_declare_background_subscriber(z_loan(session), z_loan(pong), z_move(respond), NULL) < 0) {
        printf("Unable to declare subscriber for key expression.\n");
        return -1;
    }

    uint8_t* data = z_malloc(pkt_size);
    for (unsigned int i = 0; i < pkt_size; i++) {
        data[i] = (uint8_t)(i % 10);
    }

    // Create payload
    unsigned long prev_val = sync_tx_rx;
    z_owned_bytes_t payload;
    z_bytes_from_buf(&payload, data, pkt_size, NULL, NULL);
    z_owned_bytes_t curr_payload;
    if (warmup_ms) {
        z_clock_t warmup_start = z_clock_now();
        unsigned long elapsed_us = 0;
        while (elapsed_us < warmup_ms * 1000) {
            z_bytes_clone(&curr_payload, z_loan(payload));
            if (z_publisher_put(z_loan(pub), z_move(curr_payload), NULL) != _Z_RES_OK) {
                printf("Tx failed");
                continue;
            }
            prev_val = load_loop(prev_val + 1);
            elapsed_us = z_clock_elapsed_us(&warmup_start);
        }
    }
    unsigned long* results = z_malloc(sizeof(unsigned long) * ping_nb);
    for (unsigned int i = 0; i < ping_nb; i++) {
        z_clock_t measure_start = z_clock_now();
        z_bytes_clone(&curr_payload, z_loan(payload));
        if (z_publisher_put(z_loan(pub), z_move(curr_payload), NULL) != _Z_RES_OK) {
            printf("Tx failed");
            continue;
        }
        prev_val = load_loop(prev_val + 1);
        results[i] = z_clock_elapsed_us(&measure_start);
    }
    for (unsigned int i = 0; i < ping_nb; i++) {
        printf("%lu\n", results[i]);
    }
    z_drop(z_move(payload));
    z_free(results);
    z_free(data);
    z_drop(z_move(pub));
    z_drop(z_move(session));
}

static int parse_args(int argc, char** argv, z_owned_config_t* config, unsigned int* size, unsigned int* ping_nb,
                      unsigned int* warmup_ms, bool* is_peer) {
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
    printf(
        "ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION or Z_FEATURE_PUBLICATION or "
        "Z_FEATURE_MULTI_THREAD but this example requires them.\n");
    return -2;
}
#endif
