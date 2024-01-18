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
#include "zenoh-pico/system/platform.h"

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1

#define DEFAULT_PKT_SIZE 8
#define DEFAULT_PING_NB 100
#define DEFAULT_WARMUP_MS 1000

static zp_condvar_t cond;
static zp_mutex_t mutex;

void callback(const z_sample_t* sample, void* context) {
    (void)sample;
    (void)context;
    zp_condvar_signal(&cond);
}
void drop(void* context) {
    (void)context;
    zp_condvar_free(&cond);
}

struct args_t {
    unsigned int size;             // -s
    unsigned int number_of_pings;  // -n
    unsigned int warmup_ms;        // -w
    uint8_t help_requested;        // -h
};
struct args_t parse_args(int argc, char** argv);

int main(int argc, char** argv) {
    struct args_t args = parse_args(argc, argv);
    if (args.help_requested) {
        printf(
            "\
		-n (optional, int, default=%d): the number of pings to be attempted\n\
		-s (optional, int, default=%d): the size of the payload embedded in the ping and repeated by the pong\n\
		-w (optional, int, default=%d): the warmup time in ms during which pings will be emitted but not measured\n\
		-c (optional, string): the path to a configuration file for the session. If this option isn't passed, the default configuration will be used.\n\
		",
            DEFAULT_PKT_SIZE, DEFAULT_PING_NB, DEFAULT_WARMUP_MS);
        return 1;
    }
    zp_mutex_init(&mutex);
    zp_condvar_init(&cond);
    z_owned_config_t config = z_config_default();
    z_owned_session_t session = z_open(z_move(config));
    if (!z_check(session)) {
        printf("Unable to open session!\n");
        return -1;
    }

    if (zp_start_read_task(z_loan(session), NULL) < 0 || zp_start_lease_task(z_loan(session), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&session));
        return -1;
    }

    z_keyexpr_t ping = z_keyexpr_unchecked("test/ping");
    z_owned_publisher_t pub = z_declare_publisher(z_loan(session), ping, NULL);
    if (!z_check(pub)) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }

    z_keyexpr_t pong = z_keyexpr_unchecked("test/pong");
    z_owned_closure_sample_t respond = z_closure(callback, drop, NULL);
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(session), pong, z_move(respond), NULL);
    if (!z_check(sub)) {
        printf("Unable to declare subscriber for key expression.\n");
        return -1;
    }

    uint8_t* data = zp_malloc(args.size);
    for (unsigned int i = 0; i < args.size; i++) {
        data[i] = i % 10;
    }
    zp_mutex_lock(&mutex);
    if (args.warmup_ms) {
        printf("Warming up for %dms...\n", args.warmup_ms);
        zp_clock_t warmup_start = zp_clock_now();
        unsigned long elapsed_us = 0;
        while (elapsed_us < args.warmup_ms * 1000) {
            z_publisher_put(z_loan(pub), data, args.size, NULL);
            zp_condvar_wait(&cond, &mutex);
            elapsed_us = zp_clock_elapsed_us(&warmup_start);
        }
    }
    unsigned long* results = zp_malloc(sizeof(unsigned long) * args.number_of_pings);
    for (unsigned int i = 0; i < args.number_of_pings; i++) {
        zp_clock_t measure_start = zp_clock_now();
        z_publisher_put(z_loan(pub), data, args.size, NULL);
        zp_condvar_wait(&cond, &mutex);
        results[i] = zp_clock_elapsed_us(&measure_start);
    }
    for (unsigned int i = 0; i < args.number_of_pings; i++) {
        printf("%d bytes: seq=%d rtt=%luus, lat=%luus\n", args.size, i, results[i], results[i] / 2);
    }
    zp_mutex_unlock(&mutex);
    zp_free(results);
    zp_free(data);
    z_drop(z_move(pub));
    z_drop(z_move(sub));

    zp_stop_read_task(z_loan(session));
    zp_stop_lease_task(z_loan(session));

    z_close(z_move(session));
}

char* getopt(int argc, char** argv, char option) {
    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]);
        if (len >= 2 && argv[i][0] == '-' && argv[i][1] == option) {
            if (len > 2 && argv[i][2] == '=') {
                return argv[i] + 3;
            } else if (i + 1 < argc) {
                return argv[i + 1];
            }
        }
    }
    return NULL;
}

struct args_t parse_args(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            return (struct args_t){.help_requested = 1};
        }
    }
    char* arg = getopt(argc, argv, 's');
    unsigned int size = DEFAULT_PKT_SIZE;
    if (arg) {
        size = atoi(arg);
    }
    arg = getopt(argc, argv, 'n');
    unsigned int number_of_pings = DEFAULT_PING_NB;
    if (arg) {
        number_of_pings = atoi(arg);
    }
    arg = getopt(argc, argv, 'w');
    unsigned int warmup_ms = DEFAULT_WARMUP_MS;
    if (arg) {
        warmup_ms = atoi(arg);
    }
    return (struct args_t){
        .help_requested = 0,
        .size = size,
        .number_of_pings = number_of_pings,
        .warmup_ms = warmup_ms,
    };
}
#else
int main(void) {
    printf(
        "ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION or Z_FEATURE_PUBLICATION but this example "
        "requires them.\n");
    return -2;
}
#endif
