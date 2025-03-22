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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_LIVELINESS == 1

static volatile int msg_nb = 0;

static int parse_args(int argc, char **argv, z_owned_config_t *config, char **keyexpr, int *n, bool *history);

void data_handler(z_loaned_sample_t *sample, void *ctx) {
    (void)(ctx);
    z_view_string_t key_string;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &key_string);
    switch (z_sample_kind(sample)) {
        case Z_SAMPLE_KIND_PUT:
            printf(">> [LivelinessSubscriber] New alive token ('%.*s')\n", (int)z_string_len(z_loan(key_string)),
                   z_string_data(z_loan(key_string)));
            break;
        case Z_SAMPLE_KIND_DELETE:
            printf(">> [LivelinessSubscriber] Dropped token ('%.*s')\n", (int)z_string_len(z_loan(key_string)),
                   z_string_data(z_loan(key_string)));
            break;
    }
    msg_nb++;
}

int main(int argc, char **argv) {
    char *keyexpr = "group1/**";
    bool history = false;
    int n = 0;

    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config, &keyexpr, &n, &history);
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
        z_session_drop(z_session_move(&s));
        return -1;
    }

    printf("Declaring liveliness subscriber on '%s'...\n", keyexpr);
    z_owned_closure_sample_t callback;
    z_closure(&callback, data_handler, NULL, NULL);
    z_owned_subscriber_t sub;

    z_liveliness_subscriber_options_t sub_opt;
    z_liveliness_subscriber_options_default(&sub_opt);
    sub_opt.history = history;

    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression\n", keyexpr);
        return -1;
    }
    if (z_liveliness_declare_subscriber(z_loan(s), &sub, z_loan(ke), z_move(callback), &sub_opt) < 0) {
        printf("Unable to declare liveliness subscriber.\n");
        exit(-1);
    }

    printf("Press CTRL-C to quit...\n");
    while (1) {
        if (n != 0 && msg_nb >= n) {
            break;
        }
        z_sleep_s(1);
    }

    // Clean up
    z_drop(z_move(sub));
    z_drop(z_move(s));
    return 0;
}

// Note: All args can be specified multiple times. For "-e" it will append the list of endpoints, for the other it will
// simply replace the previous value.
static int parse_args(int argc, char **argv, z_owned_config_t *config, char **keyexpr, int *n, bool *history) {
    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:l:n:h")) != -1) {
        switch (opt) {
            case 'k':
                *keyexpr = optarg;
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
            case 'n':
                *n = atoi(optarg);
                break;
            case 'h':
                *history = true;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm' || optopt == 'l' || optopt == 'n') {
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
        "ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION and Z_FEATURE_LIVELINESS but this example "
        "requires it.\n");
    return -2;
}
#endif
