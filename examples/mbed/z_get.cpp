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

#include <EthernetInterface.h>
#include <mbed.h>
#include <randLIB.h>
#include <zenoh-pico.h>

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

void reply_dropper(void *ctx) { printf(" >> Received query final notification\n"); }

void reply_handler(const z_loaned_reply_t *oreply, void *ctx) {
    if (z_reply_is_ok(oreply)) {
        const z_loaned_sample_t *sample = z_reply_ok(oreply);
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t replystr;
        z_bytes_deserialize_into_string(z_sample_payload(sample), &replystr);

        printf(" >> Received ('%s': '%s')\n", z_string_data(z_view_string_loan(&keystr)),
               z_string_data(z_string_loan(&replystr)));
        z_string_drop(z_string_move(&replystr));
    } else {
        printf(" >> Received an error\n");
    }
}

int main(int argc, char **argv) {
    randLIB_seed_random();

    EthernetInterface net;
    net.set_network("192.168.11.2", "255.255.255.0", "192.168.11.1");
    net.connect();

    // Initialize Zenoh Session and other parameters
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, MODE);
    if (strcmp(CONNECT, "") != 0) {
        zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, CONNECT);
    }

    // Open Zenoh session
    printf("Opening Zenoh Session...");
    z_owned_session_t s;
    if (z_open(&s, z_config_move(&config)) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    printf("OK\n");

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_session_loan_mut(&s), NULL);
    zp_start_lease_task(z_session_loan_mut(&s), NULL);

    while (1) {
        z_sleep_s(5);
        printf("Sending Query '%s'...\n", KEYEXPR);
        z_get_options_t opts;
        z_get_options_default(&opts);
        // Value encoding
        z_owned_bytes_t payload;
        if (strcmp(VALUE, "") != 0) {
            z_bytes_serialize_from_str(&payload, VALUE);
            opts.payload = &payload;
        }
        z_owned_closure_reply_t callback;
        z_closure_reply(&callback, reply_handler, reply_dropper, NULL);
        z_view_keyexpr_t ke;
        z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
        if (z_get(z_session_loan(&s), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&callback), &opts) < 0) {
            printf("Unable to send query.\n");
            exit(-1);
        }
    }

    printf("Closing Zenoh Session...");

    z_close(z_session_move(&s));
    printf("OK!\n");

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERY but this example requires it\n");
    return -2;
}
#endif
