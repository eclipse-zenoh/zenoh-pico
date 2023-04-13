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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <zenoh-pico.h>

// time in seconds to see how many messages can be put
#define TEST_PERIOD 10

double double_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    double dt = ts.tv_sec + ts.tv_nsec * 1e-9;
    return dt;
}

int main(int argc, char **argv) {
    const char *keyexpr = "demo/example/zenoh-pico-put";
    const char *value = "Pub from Pico!";
    const char *mode = "client";
    char *locator = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "k:v:e:m:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'v':
                value = optarg;
                break;
            case 'e':
                locator = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'v' || optopt == 'e' || optopt == 'm') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }

    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    if (locator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_PEER_KEY, z_string_make(locator));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        return -1;
    }

    printf("Declaring multiple key expressions...\n");
    z_owned_keyexpr_t kes[30];
    for (size_t i=0;i<30;i++)
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%s/%zu", keyexpr, i);
        kes[i] = z_declare_keyexpr(z_loan(s), z_keyexpr(buffer));
        if(!z_check(kes[i])) {
            printf("Unable to declare key expression! %zu\n", i);
            return -1;
        }
    }

    printf("Testing sequential puts in %d seconds...\n", TEST_PERIOD);
    size_t messages_published = 0;
    double started = double_time();
    z_put_options_t options = z_put_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
    bool working = true;
    while(((double_time() - started) < TEST_PERIOD) && working)
    {
        uint8_t payload_bufs[30][128];
        uint8_t *payloads[30];
        z_zint_t payload_lens[30];
        z_keyexpr_t keys[30];
        for (size_t i=0;i<30;i++) {
            snprintf((char *)payload_bufs[i], 128, "[%16zu] %s", messages_published++, value);
            payloads[i] = payload_bufs[i];
            payload_lens[i] = strlen((char *)payloads[i]);
            keys[i] = z_loan(kes[i]);
        }
        for (size_t i=0;i<30;i++) {
            if (z_put(z_loan(s), keys[i], payloads[i], payload_lens[i], &options) < 0) {
                printf("Oh no! Put has failed...\n");
                working = false;
                break;
            }
        }
    }
    double ended = double_time();
    printf("Published %zu messages in %f seconds\n", messages_published, (ended - started));

    printf("Testing batched puts in %d seconds...\n", TEST_PERIOD);
    messages_published = 0;
    started = double_time();
    working = true;
    while(((double_time() - started) < TEST_PERIOD) && working)
    {
        uint8_t payload_bufs[30][128];
        uint8_t *payloads[30];
        z_zint_t payload_lens[30];
        z_keyexpr_t keys[30];
        for (size_t i=0;i<30;i++) {
            snprintf((char *)payload_bufs[i], 128, "[%16zu] %s", messages_published++, value);
            payloads[i] = payload_bufs[i];
            payload_lens[i] = strlen((char *)payloads[i]);
            keys[i] = z_loan(kes[i]);
        }
        if (z_put_multi(z_loan(s), keys, (const uint8_t **)payloads, payload_lens, 30, &options) < 0) {
            printf("Oh no! Put has failed...\n");
            working = false;
            break;
        }
    }
    ended = double_time();
    printf("Published %zu messages in %f seconds \n", messages_published, (ended - started));

    for (size_t i=0;i<30;i++) {
        z_undeclare_keyexpr(z_loan(s), z_move(kes[i]));
    }

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));

    z_close(z_move(s));
    return 0;
}
