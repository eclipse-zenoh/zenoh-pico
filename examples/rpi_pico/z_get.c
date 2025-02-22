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

#include <stdio.h>
#include <zenoh-pico.h>

#include "config.h"

#if Z_FEATURE_QUERY == 1

#define KEYEXPR "demo/example/**"
#define VALUE ""

void reply_dropper(void *ctx) {
    (void)(ctx);
    printf(">> Received query final notification\n");
}

void reply_handler(z_loaned_reply_t *reply, void *ctx) {
    (void)(ctx);
    if (z_reply_is_ok(reply)) {
        const z_loaned_sample_t *sample = z_reply_ok(reply);
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t replystr;
        z_bytes_to_string(z_sample_payload(sample), &replystr);

        printf(">> Received ('%.*s': '%.*s')\n", (int)z_string_len(z_loan(keystr)), z_string_data(z_loan(keystr)),
               (int)z_string_len(z_loan(replystr)), z_string_data(z_loan(replystr)));
        z_drop(z_move(replystr));
    } else {
        printf(">> Received an error\n");
    }
}

void app_main(void) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, ZENOH_CONFIG_MODE);
    if (strcmp(ZENOH_CONFIG_CONNECT, "") != 0) {
        printf("Connect endpoint: %s\n", ZENOH_CONFIG_CONNECT);
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, ZENOH_CONFIG_CONNECT);
    }
    if (strcmp(ZENOH_CONFIG_LISTEN, "") != 0) {
        printf("Listen endpoint: %s\n", ZENOH_CONFIG_LISTEN);
        zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, ZENOH_CONFIG_LISTEN);
    }

    printf("Opening %s session ...\n", ZENOH_CONFIG_MODE);
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_drop(z_move(s));
        return;
    }

    z_view_keyexpr_t ke;
    if (z_view_keyexpr_from_str(&ke, KEYEXPR) < 0) {
        printf("%s is not a valid key expression\n", KEYEXPR);
        return;
    }

    while (1) {
        z_sleep_s(5);
        printf("Sending Query '%s'...\n", KEYEXPR);
        z_get_options_t opts;
        z_get_options_default(&opts);
        // Value encoding
        z_owned_bytes_t payload;
        if (strcmp(VALUE, "") != 0) {
            z_bytes_from_static_str(&payload, VALUE);
            opts.payload = z_move(payload);
        }
        z_owned_closure_reply_t callback;
        z_closure(&callback, reply_handler, reply_dropper, NULL);
        if (z_get(z_loan(s), z_loan(ke), "", z_move(callback), &opts) < 0) {
            printf("Unable to send query.\n");
            return;
        }
    }

    z_drop(z_move(s));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERY but this example requires it.\n");
}
#endif
