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

#if Z_FEATURE_QUERYABLE == 1
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

#define KEYEXPR "demo/example/zenoh-pico-queryable"
#define VALUE "[MBedOS]{nucleo-F767ZI} Queryable from Zenoh-Pico!"

void query_handler(const z_loaned_query_t *query, void *ctx) {
    (void)(ctx);
    z_owned_string_t keystr;
    z_keyexpr_to_string(z_query_keyexpr(query), &keystr);
    z_view_string_t pred;
    z_query_parameters(query, &pred);
    printf(" >> [Queryable handler] Received Query '%s%.*s'\n", z_str_data(z_string_loan(&keystr)),
           (int)z_view_string_loan(&pred)->len, z_view_string_loan(&pred)->val);
    // Process value
    const z_loaned_bytes_t *payload = z_value_payload(z_query_value(query));
    if (z_bytes_len(payload) > 0) {
        z_owned_string_t payload_string;
        z_bytes_decode_into_string(payload, &payload_string);
        printf("     with value '%s'\n", z_str_data(z_string_loan(&payload_string)));
        z_string_drop(z_string_move(&payload_string));
    }
    // Reply value encoding
    z_view_string_t reply_str;
    z_view_str_wrap(&reply_str, VALUE);
    z_owned_bytes_t reply_payload;
    z_bytes_encode_from_string(&reply_payload, z_view_string_loan(&reply_str));

    z_query_reply(query, z_query_keyexpr(query), z_bytes_move(&reply_payload), NULL);
    z_string_drop(z_string_move(&keystr));
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

    // Declare Zenoh queryable
    printf("Declaring Queryable on %s...", KEYEXPR);
    z_owned_closure_query_t callback;
    z_closure_query(&callback, query_handler, NULL, NULL);
    z_owned_queryable_t qable;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_string_unchecked(&ke, KEYEXPR);
    if (z_declare_queryable(&qable, z_session_loan(&s), z_view_keyexpr_loan(&ke), z_closure_query_move(&callback),
                            NULL) < 0) {
        printf("Unable to declare queryable.\n");
        exit(-1);
    }
    printf("OK\n");
    printf("Zenoh setup finished!\n");

    while (1) {
        z_sleep_s(1);
    }

    printf("Closing Zenoh Session...");
    z_undeclare_queryable(z_queryable_move(&qable));

    // Stop the receive and the session lease loop for zenoh-pico
    zp_stop_read_task(z_session_loan_mut(&s));
    zp_stop_lease_task(z_session_loan_mut(&s));

    z_close(z_session_move(&s));
    printf("OK!\n");

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERYABLE but this example requires it.\n");
    return -2;
}
#endif
