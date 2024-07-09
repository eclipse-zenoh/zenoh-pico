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

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_QUERY == 1 && Z_FEATURE_MULTI_THREAD == 1
z_owned_condvar_t cond;
z_owned_mutex_t mutex;

void reply_dropper(void *ctx) {
    (void)(ctx);
    printf(">> Received query final notification\n");
    z_condvar_signal(z_condvar_loan_mut(&cond));
    z_condvar_drop(z_condvar_move(&cond));
}

void reply_handler(const z_loaned_reply_t *reply, void *ctx) {
    (void)(ctx);
    if (z_reply_is_ok(reply)) {
        const z_loaned_sample_t *sample = z_reply_ok(reply);
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t replystr;
        z_bytes_deserialize_into_string(z_sample_payload(sample), &replystr);

        printf(">> Received ('%s': '%s')\n", z_string_data(z_view_string_loan(&keystr)),
               z_string_data(z_string_loan(&replystr)));
        z_string_drop(z_string_move(&replystr));
    } else {
        printf(">> Received an error\n");
    }
}

int main(int argc, char **argv) {
    const char *keyexpr = "demo/example/**";
    const char *mode = "client";
    const char *clocator = NULL;
    const char *llocator = NULL;
    const char *value = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "k:e:m:v:l:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'e':
                clocator = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case 'l':
                llocator = optarg;
                break;
            case 'v':
                value = optarg;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e' || optopt == 'm' || optopt == 'v' || optopt == 'l') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }

    z_mutex_init(&mutex);
    z_condvar_init(&cond);

    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, mode);
    if (clocator != NULL) {
        zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, clocator);
    }
    if (llocator != NULL) {
        zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_LISTEN_KEY, llocator);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_config_move(&config)) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan_mut(&s), NULL) < 0 || zp_start_lease_task(z_session_loan_mut(&s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
        return -1;
    }

    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, keyexpr) < 0) {
        printf("%s is not a valid key expression", keyexpr);
        return -1;
    }

    z_mutex_lock(z_mutex_loan_mut(&mutex));
    printf("Sending Query '%s'...\n", keyexpr);
    z_get_options_t opts;
    z_get_options_default(&opts);
    // Value encoding
    z_owned_bytes_t payload;
    if (value != NULL) {
        z_bytes_serialize_from_str(&payload, value);
        opts.payload = &payload;
    }
    z_owned_closure_reply_t callback;
    z_closure_reply(&callback, reply_handler, reply_dropper, NULL);
    if (z_get(z_session_loan(&s), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&callback), &opts) < 0) {
        printf("Unable to send query.\n");
        return -1;
    }
    z_condvar_wait(z_condvar_loan_mut(&cond), z_mutex_loan_mut(&mutex));
    z_mutex_unlock(z_mutex_loan_mut(&mutex));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_session_loan_mut(&s));
    zp_stop_lease_task(z_session_loan_mut(&s));

    z_close(z_session_move(&s));

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
