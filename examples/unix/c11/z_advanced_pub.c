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
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_ADVANCED_PUBLICATION == 1

static int parse_args(int argc, char **argv, z_owned_config_t *config, char **keyexpr, char **value, size_t *history);

int main(int argc, char **argv) {
    char *keyexpr = "demo/example/zenoh-c-pub";
    char *const default_value = "Pub from C!";
    char *value = default_value;
    size_t history = 1;

    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config, &keyexpr, &value, &history);
    if (ret != 0) {
        return ret;
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_drop(z_move(s));
        return -1;
    }

    // Declare advanced publisher
    printf("Declaring AdvancedPublisher for '%s'...\n", keyexpr);
    ze_owned_advanced_publisher_t pub;
    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        return -1;
    }

    ze_advanced_publisher_options_t pub_opts;
    ze_advanced_publisher_options_default(&pub_opts);
    pub_opts.cache.max_samples = history;
    pub_opts.cache.is_enabled = true;
    pub_opts.publisher_detection = true;
    pub_opts.sample_miss_detection.is_enabled = true;
    pub_opts.sample_miss_detection.heartbeat_period_ms = 500;
    pub_opts.sample_miss_detection.heartbeat_mode = ZE_ADVANCED_PUBLISHER_HEARTBEAT_MODE_PERIODIC;

    if (ze_declare_advanced_publisher(z_loan(s), &pub, z_loan(ke), &pub_opts) < 0) {
        printf("Unable to declare advanced publisher for key expression!\n");
        return -1;
    }

    // Publish data
    printf("Press CTRL-C to quit...\n");
    char buf[256];
    for (int idx = 0; 1; ++idx) {
        z_sleep_s(1);
        sprintf(buf, "[%4d] %s", idx, value);
        printf("Putting Data ('%s': '%s')...\n", keyexpr, buf);

        // Create payload
        z_owned_bytes_t payload;
        z_bytes_copy_from_str(&payload, buf);
        ze_advanced_publisher_put(z_loan(pub), z_move(payload), NULL);
    }

    z_drop(z_move(pub));
    z_drop(z_move(s));
    return 0;
}

// Note: All args can be specified multiple times. For "-e" it will append the list of endpoints, for the other it will
// simply replace the previous value.
static int parse_args(int argc, char **argv, z_owned_config_t *config, char **keyexpr, char **value, size_t *history) {
    int opt;
    while ((opt = getopt(argc, argv, "k:v:e:m:l:i:")) != -1) {
        switch (opt) {
            case 'k':
                *keyexpr = optarg;
                break;
            case 'v':
                *value = optarg;
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
            case 'i': {
                int tmp = atoi(optarg);
                if (tmp < 0) {
                    fprintf(stderr, "History size must be a positive integer.\n");
                    return 1;
                }
                *history = (size_t)tmp;
                break;
            }
            case '?':
                if (optopt == 'k' || optopt == 'v' || optopt == 'e' || optopt == 'm' || optopt == 'l' ||
                    optopt == 'i') {
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
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_ADVANCED_PUBLICATION but this example requires it.\n");
    return -2;
}
#endif
