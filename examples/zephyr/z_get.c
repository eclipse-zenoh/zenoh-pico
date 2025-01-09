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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_QUERY == 1
#define CLIENT_OR_PEER 0  // 0: Client mode; 1: Peer mode
#if CLIENT_OR_PEER == 0
#define MODE "client"
#define LOCATOR ""  // If empty, it will scout
#elif CLIENT_OR_PEER == 1
#define MODE "peer"
#define LOCATOR "udp/224.0.0.225:7447#iface=en0"
#else
#error "Unknown Zenoh operation mode. Check CLIENT_OR_PEER value."
#endif

#define KEYEXPR "demo/example/**"
#define VALUE ""

void reply_dropper(void *ctx) { printf(" >> Received query final notification\n"); }

void reply_handler(z_loaned_reply_t *oreply, void *ctx) {
    if (z_reply_is_ok(oreply)) {
        const z_loaned_sample_t *sample = z_reply_ok(oreply);
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t replystr;
        z_bytes_to_string(z_sample_payload(sample), &replystr);

        printf(" >> Received ('%.*s': '%.*s')\n", (int)z_string_len(z_loan(keystr)), z_string_data(z_loan(keystr)),
               (int)z_string_len(z_loan(replystr)), z_string_data(z_loan(replystr)));
        z_drop(z_move(replystr));
    } else {
        printf(" >> Received an error\n");
    }
}

int main(int argc, char **argv) {
    sleep(5);

    // Initialize Zenoh Session and other parameters
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);

    if (strcmp(LOCATOR, "") != 0) {
        if (strcmp(MODE, "client") == 0) {
            zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, LOCATOR);
        } else {
            zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, LOCATOR);
        }
    }

    // Open Zenoh session
    printf("Opening Zenoh Session...");
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }
    printf("OK\n");

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_loan_mut(s), NULL);
    zp_start_lease_task(z_loan_mut(s), NULL);

    while (1) {
        sleep(5);
        printf("Sending Query '%s'...\n", KEYEXPR);
        z_get_options_t opts;
        z_get_options_default(&opts);
        opts.target = Z_QUERY_TARGET_ALL;
        // Value encoding
        z_owned_bytes_t payload;
        if (strcmp(VALUE, "") != 0) {
            z_bytes_from_static_str(&payload, VALUE);
            opts.payload = z_move(payload);
        }
        z_owned_closure_reply_t callback;
        z_closure(&callback, reply_handler, reply_dropper, NULL);
        z_view_keyexpr_t ke;
        z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
        if (z_get(z_loan(s), z_loan(ke), "", z_move(callback), &opts) < 0) {
            printf("Unable to send query.\n");
            return -1;
        }
    }

    printf("Closing Zenoh Session...");

    z_drop(z_move(s));
    printf("OK!\n");

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERY but this example requires it.\n");
    return -2;
}
#endif
