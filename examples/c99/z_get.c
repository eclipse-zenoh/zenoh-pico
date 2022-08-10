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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "zenoh-pico.h"

void reply_dropper(void *ctx)
{
    (void) (ctx);
    printf(">> Received query final notification\n");
}

void reply_handler(z_owned_reply_t oreply, void *ctx)
{
    (void) (ctx);
    if (z_reply_is_ok(&oreply)) {
        z_sample_t sample = z_reply_ok(&oreply);
        char *keystr = z_keyexpr_to_string(sample.keyexpr);
        printf(">> Received ('%s': '%.*s')\n", keystr, (int)sample.payload.len, sample.payload.start);
        free(keystr);
    } else {
        printf(">> Received an error\n");
    }
}

int main(int argc, char **argv)
{
    z_init_logger();

    char *keyexpr = "demo/example/**";
    char *locator = NULL;

    int opt;
    while ((opt = getopt (argc, argv, "k:e:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'e':
                locator = optarg;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'e') {
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                exit(-1);
        }
    }

    z_owned_config_t config = zp_config_default();
    if (locator != NULL) {
        zp_config_insert(z_config_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(locator));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_config_move(&config));
    if (!z_session_check(&s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan(&s)) < 0 || zp_start_lease_task(z_session_loan(&s)) < 0) {
        printf("Unable to start read and lease tasks");
        exit(-1);
    }

    z_keyexpr_t ke = z_keyexpr(keyexpr);
    if (!z_keyexpr_is_valid(&ke)) {
        printf("%s is not a valid key expression", keyexpr);
        exit(-1);
    }

    printf("Enter any key to pull data or 'q' to quit...\n");
    char c = 0;
    while (1) {
        c = getchar();
        if (c == 'q') {
            break;
        }

        printf("Sending Query '%s'...\n", keyexpr);
        z_get_options_t opts = z_get_options_default();
        opts.target = Z_QUERY_TARGET_ALL;
        z_owned_closure_reply_t callback = z_closure_reply(reply_handler, reply_dropper, NULL);
        if (z_get(z_session_loan(&s), ke, "", z_closure_reply_move(&callback), &opts) < 0) {
            printf("Unable to send query.\n");
            exit(-1);
        }
    }

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_session_loan(&s));
    zp_stop_lease_task(z_session_loan(&s));

    z_close(z_session_move(&s));

    return 0;
}
