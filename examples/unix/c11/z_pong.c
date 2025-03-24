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

#include <stdio.h>
#include <unistd.h>

#include "zenoh-pico.h"

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1

static int parse_args(int argc, char** argv, z_owned_config_t* config);

void callback(z_loaned_sample_t* sample, void* context) {
    const z_loaned_publisher_t* pub = z_loan(*(z_owned_publisher_t*)context);
    z_owned_bytes_t payload = {._val = sample->payload};
    z_publisher_put(pub, z_move(payload), NULL);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    z_owned_config_t config;
    z_config_default(&config);

    int ret = parse_args(argc, argv, &config);
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
    if (argc <= 1) {
        if (zp_start_lease_task(z_loan_mut(session), NULL) < 0) {
            printf("Unable to start lease tasks\n");
            z_drop(z_move(session));
            return -1;
        }
    }

    z_view_keyexpr_t pong;
    z_view_keyexpr_from_str_unchecked(&pong, "test/pong");
    z_owned_publisher_t pub;
    if (z_declare_publisher(z_loan(session), &pub, z_loan(pong), NULL) < 0) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }

    z_view_keyexpr_t ping;
    z_view_keyexpr_from_str_unchecked(&ping, "test/ping");
    z_owned_closure_sample_t respond;
    z_closure(&respond, callback, NULL, (void*)(&pub));
    if (z_declare_background_subscriber(z_loan(session), z_loan(ping), z_move(respond), NULL) < 0) {
        printf("Unable to declare subscriber for key expression.\n");
        return -1;
    }
    printf("Pong app is running, press 'q' to quit\n");
    while (getchar() != 'q') {
    }
    z_drop(z_move(pub));
    z_drop(z_move(session));
}

static int parse_args(int argc, char** argv, z_owned_config_t* config) {
    int opt;
    while ((opt = getopt(argc, argv, "e:m:l:")) != -1) {
        switch (opt) {
            case 'e':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_CONNECT_KEY, optarg);
                break;
            case 'm':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_MODE_KEY, optarg);
                break;
            case 'l':
                zp_config_insert(z_loan_mut(*config), Z_CONFIG_LISTEN_KEY, optarg);
                break;
            case '?':
                if (optopt == 'e' || optopt == 'm' || optopt == 'l') {
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
        "ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION or Z_FEATURE_PUBLICATION but this example "
        "requires them.\n");
    return -2;
}
#endif
