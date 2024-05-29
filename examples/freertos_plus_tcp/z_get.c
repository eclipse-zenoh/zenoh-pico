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
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <zenoh-pico.h>

#include "FreeRTOS.h"

#if Z_FEATURE_QUERY == 1
#define CLIENT_OR_PEER 0  // 0: Client mode; 1: Peer mode
#if CLIENT_OR_PEER == 0
#define MODE "client"
#define CONNECT ""  // If empty, it will scout
#elif CLIENT_OR_PEER == 1
#define MODE "peer"
#define CONNECT "udp/224.0.0.225:7447#iface=en0"
#else
#error "Unknown Zenoh operation mode. Check CLIENT_OR_PEER value."
#endif

#define KEYEXPR "demo/example/**"
#define VALUE ""

void reply_dropper(void *ctx) {
    (void)(ctx);
    printf(">> Received query final notification\n");
}

void reply_handler(const z_loaned_reply_t *reply, void *ctx) {
    (void)(ctx);
    if (z_reply_is_ok(reply)) {
        const z_loaned_sample_t *sample = z_reply_ok(reply);
        z_owned_string_t keystr;
        z_keyexpr_to_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t replystr;
        z_bytes_decode_into_string(z_sample_payload(sample), &replystr);

        printf(">> Received ('%s': '%s')\n", z_str_data(z_loan(keystr)), z_str_data(z_loan(replystr)));
        z_drop(z_move(keystr));
        z_drop(z_move(replystr));
    } else {
        printf(">> Received an error\n");
    }
}

void app_main(void) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
    if (strcmp(CONNECT, "") != 0) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, CONNECT);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config)) < 0) {
        printf("Unable to open session!\n");
        return;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&s));
        return;
    }

    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_string(&ke, KEYEXPR) < 0) {
        printf("%s is not a valid key expression", KEYEXPR);
        return -1;
    }

    while (1) {
        z_sleep_s(5);
        printf("Sending Query '%s'...\n", KEYEXPR);
        z_get_options_t opts;
        z_get_options_default(&opts);
        // Value encoding
        if (strcmp(VALUE, "") != 0) {
            z_view_string_t value_str;
            z_view_str_wrap(&value_str, VALUE);
            z_owned_bytes_t payload;
            z_bytes_encode_from_string(&payload, z_loan(value_str));
            opts.payload = &payload;
        }
        z_owned_closure_reply_t callback;
        z_closure(&callback, reply_handler, reply_dropper);
        if (z_get(z_loan(s), z_loan(ke), "", z_move(callback), &opts) < 0) {
            printf("Unable to send query.\n");
            return;
        }
    }

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_loan_mut(s));
    zp_stop_lease_task(z_loan_mut(s));

    z_close(z_move(s));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERY but this example requires it.\n");
}
#endif
