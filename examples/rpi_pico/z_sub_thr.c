//
// Copyright (c) 2024 ZettaScale Technology
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zenoh-pico.h>

#include "config.h"

#if Z_FEATURE_SUBSCRIPTION == 1

#define KEYEXPR "test/thr"

#define PACKET_NB 1000000

typedef struct {
    volatile unsigned long count;
    volatile unsigned long finished_rounds;
    z_clock_t start;
    z_clock_t first_start;
} z_stats_t;

z_stats_t *z_stats_make(void) {
    z_stats_t *stats = malloc(sizeof(z_stats_t));
    stats->count = 0;
    stats->finished_rounds = 0;
    stats->first_start.tv_nsec = 0;
    return stats;
}

void on_sample(z_loaned_sample_t *sample, void *context) {
    (void)sample;
    z_stats_t *stats = (z_stats_t *)context;
    stats->count++;
    // Start set measurement
    if (stats->count == 1) {
        stats->start = z_clock_now();
        if (stats->first_start.tv_nsec == 0) {
            stats->first_start = stats->start;
        }
    } else if (stats->count >= PACKET_NB) {
        // Stop set measurement
        stats->finished_rounds++;
        unsigned long elapsed_ms = z_clock_elapsed_ms(&stats->start);
        printf("%f msg/s\n", (double)(PACKET_NB * 1000) / (double)elapsed_ms);
        stats->count = 0;
    }
}

void drop_stats(void *context) {
    z_stats_t *stats = (z_stats_t *)context;
    unsigned long elapsed_ms = z_clock_elapsed_ms(&stats->first_start);
    const unsigned long sent_messages = PACKET_NB * stats->finished_rounds + stats->count;
    printf("Stats after unsubscribing: received %ld messages over %lu miliseconds (%.1f msg/s)\n", sent_messages,
           elapsed_ms, (double)(sent_messages * 1000) / (double)elapsed_ms);
    free(context);
}

void app_main(void) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, ZENOH_CONFIG_MODE);
    if (strcmp(ZENOH_CONFIG_CONNECT, "") != 0) {
        printf("Connect endpoint: %s\n", ZENOH_CONFIG_CONNECT);
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, ZENOH_CONFIG_CONNECT);
    }
    if (strcmp(ZENOH_CONFIG_LISTEN, "") != 0) {
        printf("Listen endpoint: %s\n", ZENOH_CONFIG_LISTEN);
        zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, ZENOH_CONFIG_LISTEN);
    }

    printf("Opening %s session ...\n", ZENOH_CONFIG_MODE);
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_drop(z_move(s));
        return;
    }
    // Declare Subscriber/resource
    z_stats_t *context = z_stats_make();
    z_owned_closure_sample_t callback;
    z_closure(&callback, on_sample, drop_stats, (void *)context);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    if (z_declare_background_subscriber(z_loan(s), z_loan(ke), z_move(callback), NULL) < 0) {
        printf("Unable to declare subscriber.\n");
        return;
    }

    while (1) {
        z_sleep_s(1);
    }

    // Clean-up
    z_drop(z_move(s));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
}
#endif
