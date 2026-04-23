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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rtthread.h>

#include "zenoh-pico.h"

#define KEYEXPR "demo/example/**"
#define VALUE "[RT-Thread] Get from Zenoh-Pico!"

void reply_dropper(void *ctx) {
    (void)(ctx);
}

void reply_handler(z_loaned_reply_t *reply, void *ctx) {
    (void)(ctx);
    if (z_reply_is_ok(reply)) {
        z_loaned_sample_t *sample = z_reply_ok(reply);
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t replystr;
        z_bytes_to_string(z_sample_payload(sample), &replystr);

        printf(">> Received ('%.*s': '%.*s')\n", (int)z_string_len(z_loan(keystr)),
               z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(replystr)), z_string_data(z_loan(replystr)));
        z_drop(z_move(replystr));
    } else {
        printf(">> Received an error\n");
    }
}

int main(int argc, char **argv) {
    const char *keyexpr = KEYEXPR;
    const char *value = VALUE;
    const char *mode = "client";
    char *locator = NULL;

    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, mode);
    if (locator != NULL) {
        zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, locator);
    }

    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        return -1;
    }

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_move(s), NULL);
        return -1;
    }

    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, keyexpr);
    while (1) {
        rt_thread_mdelay(5000);
        printf("Sending Query '%s'...\n", keyexpr);
        z_owned_closure_reply_t callback;
        z_closure(&callback, reply_handler, reply_dropper, NULL);
        z_get_options_t opts;
        z_get_options_default(&opts);
        z_owned_bytes_t payload;
        if (value != NULL) {
            z_bytes_from_str(&payload, value);
            opts.payload = z_move(payload);
        }
        z_owned_fifo_handler_reply_t handler;
        z_fifo_channel_reply_new(&callback, &handler, 16);
        z_get(z_loan(s), z_loan(ke), "", z_move(callback), &opts);
        z_owned_reply_t reply;
        for (z_result_t res = z_recv(z_loan(handler), &reply); res == Z_OK; res = z_recv(z_loan(handler), &reply)) {
            if (z_reply_is_ok(z_loan(reply))) {
                z_loaned_sample_t *sample = z_reply_ok(z_loan(reply));
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

    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s), NULL);
    return 0;
}

// RT-Thread MSH command
static int zenoh_get(int argc, char **argv) {
    return main(argc, argv);
}
MSH_CMD_EXPORT(zenoh_get, zenoh get example);