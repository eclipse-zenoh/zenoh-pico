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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "zenoh-pico.h"

#define PACKET_NB 1000000

typedef struct {
    volatile unsigned long count;
    volatile unsigned long finished_rounds;
    zp_clock_t start;
    zp_clock_t first_start;
} z_stats_t;

#if Z_FEATURE_SUBSCRIPTION == 1

z_stats_t *z_stats_make(void) {
    z_stats_t *stats = malloc(sizeof(z_stats_t));
    stats->count = 0;
    stats->finished_rounds = 0;
    stats->first_start.tv_nsec = 0;
    return stats;
}

void on_sample(const z_sample_t *sample, void *context) {
    (void)sample;
    z_stats_t *stats = (z_stats_t *)context;
    stats->count++;
    // Start set measurement
    if (stats->count == 1) {
        stats->start = zp_clock_now();
        if (stats->first_start.tv_nsec == 0) {
            stats->first_start = stats->start;
        }
    } else if (stats->count >= PACKET_NB) {
        // Stop set measurement
        stats->finished_rounds++;
        unsigned long elapsed_ms = zp_clock_elapsed_ms(&stats->start);
        printf("Received %d msg in %lu ms (%.1f msg/s)\n", PACKET_NB, elapsed_ms,
               (double)(PACKET_NB * 1000) / (double)elapsed_ms);
        stats->count = 0;
    }
}

void drop_stats(void *context) {
    z_stats_t *stats = (z_stats_t *)context;
    unsigned long elapsed_ms = zp_clock_elapsed_ms(&stats->first_start);
    const unsigned long sent_messages = PACKET_NB * stats->finished_rounds + stats->count;
    printf("Stats after unsubscribing: received %ld messages over %lu miliseconds (%.1f msg/s)\n", sent_messages,
           elapsed_ms, (double)(sent_messages * 1000) / (double)elapsed_ms);
    free(context);
}

int main(int argc, char **argv) {
    char *keyexpr = "test/thr";
    z_owned_config_t config = z_config_default();

    // Set config
    if (argc > 1) {
        if (zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, z_string_make(argv[1])) < 0) {
            printf("Failed to insert locator in config: %s\n", argv[1]);
            exit(-1);
        }
    }
    // Open session
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
        exit(-1);
    }
    // Declare Subscriber/resource
    z_stats_t *context = z_stats_make();
    z_owned_closure_sample_t callback = z_closure(on_sample, drop_stats, (void *)context);
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(s), z_keyexpr(keyexpr), z_move(callback), NULL);
    if (!z_check(sub)) {
        printf("Unable to create subscriber.\n");
        exit(-1);
    }
    // Listen until stopped
    printf("Start listening.\n");
    char c = 0;
    while (c != 'q') {
        c = (char)fgetc(stdin);
    }
    // Wait for everything to settle
    printf("End of test\n");
    zp_sleep_s(1);
    // Clean up
    z_undeclare_subscriber(z_move(sub));
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s));
    exit(0);
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
