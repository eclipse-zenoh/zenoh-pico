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

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_QUERY == 1 && Z_FEATURE_MULTI_THREAD == 1 && defined Z_FEATURE_UNSTABLE_API

static int parse_args(int argc, char **argv, z_owned_config_t *config, char **selector, char **value, int *n,
                      int *timeout_ms, bool *add_matching_listener);

#if Z_FEATURE_MATCHING == 1
void matching_status_handler(const z_matching_status_t *matching_status, void *arg) {
    (void)arg;
    if (matching_status->matching) {
        printf("Querier has matching queryable.\n");
    } else {
        printf("Querier has NO MORE matching queryables.\n");
    }
}
#endif

int main(int argc, char **argv) {
    char *selector = "demo/example/**";
    char *value = NULL;
    int n = INT_MAX;
    int timeout_ms = 0;
    bool add_matching_listener = false;

    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config, &selector, &value, &n, &timeout_ms, &add_matching_listener);
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

    const char *ke = selector;
    size_t ke_len = strlen(ke);
    const char *params = strchr(selector, '?');
    if (params != NULL) {
        ke_len = (size_t)(params - ke);
        params += 1;
    }

    z_view_keyexpr_t keyexpr;
    if (z_view_keyexpr_from_substr(&keyexpr, ke, ke_len) < 0) {
        printf("%.*s is not a valid key expression\n", (int)ke_len, ke);
        exit(-1);
    }

    printf("Declaring Querier on '%s'...\n", ke);
    z_owned_querier_t querier;

    z_querier_options_t opts;
    z_querier_options_default(&opts);
    opts.timeout_ms = (uint64_t)timeout_ms;

    if (z_declare_querier(z_loan(s), &querier, z_loan(keyexpr), &opts) < 0) {
        printf("Unable to declare Querier for key expression!\n");
        exit(-1);
    }

    if (add_matching_listener) {
#if Z_FEATURE_MATCHING == 1
        z_owned_closure_matching_status_t callback;
        z_closure(&callback, matching_status_handler, NULL, NULL);
        z_querier_declare_background_matching_listener(z_loan(querier), z_move(callback));
#else
        printf("ERROR: Zenoh pico was compiled without Z_FEATURE_MATCHING but this example requires it.\n");
        return -2;
#endif
    }

    printf("Press CTRL-C to quit...\n");
    char buf[256];
    for (int idx = 0; idx != n; ++idx) {
        z_sleep_s(1);
        sprintf(buf, "[%4d] %s", idx, value ? value : "");
        printf("Querying '%s' with payload '%s'...\n", selector, buf);
        z_querier_get_options_t get_options;
        z_querier_get_options_default(&get_options);

        if (value != NULL) {
            z_owned_bytes_t payload;
            z_bytes_copy_from_str(&payload, buf);
            get_options.payload = z_move(payload);
        }

        z_owned_fifo_handler_reply_t handler;
        z_owned_closure_reply_t closure;
        z_fifo_channel_reply_new(&closure, &handler, 16);

        z_querier_get(z_loan(querier), params, z_move(closure), &get_options);

        z_owned_reply_t reply;
        for (z_result_t res = z_recv(z_loan(handler), &reply); res == Z_OK; res = z_recv(z_loan(handler), &reply)) {
            if (z_reply_is_ok(z_loan(reply))) {
                const z_loaned_sample_t *sample = z_reply_ok(z_loan(reply));

                z_view_string_t keystr;
                z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);

                z_owned_string_t replystr;
                z_bytes_to_string(z_sample_payload(sample), &replystr);

                printf(">> Received ('%.*s': '%.*s')\n", (int)z_string_len(z_loan(keystr)),
                       z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(replystr)),
                       z_string_data(z_loan(replystr)));
                z_drop(z_move(replystr));
            } else {
                printf(">> Received an error\n");
            }
            z_drop(z_move(reply));
        }
        z_drop(z_move(handler));
    }

    z_drop(z_move(querier));
    z_drop(z_move(s));

    return 0;
}

// Note: All args can be specified multiple times. For "-e" it will append the list of endpoints, for the other it will
// simply replace the previous value.
static int parse_args(int argc, char **argv, z_owned_config_t *config, char **selector, char **value, int *n,
                      int *timeout_ms, bool *add_matching_listener) {
    int opt;
    while ((opt = getopt(argc, argv, "s:v:e:m:l:n:t:a")) != -1) {
        switch (opt) {
            case 's':
                *selector = optarg;
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
            case 'n':
                *n = atoi(optarg);
                break;
            case 't':
                *timeout_ms = atoi(optarg);
                break;
            case 'a':
                *add_matching_listener = true;
                break;
            case '?':
                if (optopt == 's' || optopt == 'v' || optopt == 'e' || optopt == 'm' || optopt == 'l' ||
                    optopt == 't' || optopt == 'n') {
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
        "ERROR: Zenoh pico was compiled without Z_FEATURE_QUERY or Z_FEATURE_MULTI_THREAD but this example requires "
        "them.\n");
    return -2;
}
#endif
