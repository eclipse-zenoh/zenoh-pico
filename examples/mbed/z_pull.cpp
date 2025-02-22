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

#if Z_FEATURE_SUBSCRIPTION == 1
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

const size_t INTERVAL = 5000;
const size_t SIZE = 3;

int main(int argc, char **argv) {
    randLIB_seed_random();

    EthernetInterface net;
    net.set_network("192.168.11.2", "255.255.255.0", "192.168.11.1");
    net.connect();

    // Initialize Zenoh Session and other parameters
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, MODE);
    if (strcmp(LOCATOR, "") != 0) {
        if (strcmp(MODE, "client") == 0) {
            zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, LOCATOR);
        } else {
            zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_LISTEN_KEY, LOCATOR);
        }
    }

    // Open Zenoh session
    printf("Opening Zenoh Session...");
    z_owned_session_t s;
    if (z_open(&s, z_config_move(&config), NULL) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    printf("OK\n");

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan_mut(&s), NULL) < 0 || zp_start_lease_task(z_session_loan_mut(&s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        return -1;
    }

    printf("Declaring Subscriber on '%s'...\n", KEYEXPR);
    z_owned_closure_sample_t closure;
    z_owned_ring_handler_sample_t handler;
    z_ring_channel_sample_new(&closure, &handler, SIZE);
    z_owned_subscriber_t sub;
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    if (z_declare_subscriber(z_session_loan(&s), &sub, z_view_keyexpr_loan(&ke), z_closure_sample_move(&closure),
                             NULL) < 0) {
        printf("Unable to declare subscriber.\n");
        return -1;
    }

    printf("Pulling data every %zu ms... Ring size: %zd\n", INTERVAL, SIZE);
    z_owned_sample_t sample;
    while (true) {
        z_result_t res;
        for (res = z_ring_handler_sample_try_recv(z_ring_handler_sample_loan(&handler), &sample); res == Z_OK;
             res = z_ring_handler_sample_try_recv(z_ring_handler_sample_loan(&handler), &sample)) {
            z_view_string_t keystr;
            z_keyexpr_as_view_string(z_sample_keyexpr(z_sample_loan(&sample)), &keystr);
            z_owned_string_t value;
            z_bytes_to_string(z_sample_payload(z_sample_loan(&sample)), &value);
            printf(">> [Subscriber] Pulled ('%.*s': '%.*s')\n", (int)z_string_len(z_view_string_loan(&keystr)),
                   z_string_data(z_view_string_loan(&keystr)), (int)z_string_len(z_string_loan(&value)),
                   z_string_data(z_string_loan(&value)));
            z_string_drop(z_string_move(&value));
            z_sample_drop(z_sample_move(&sample));
        }
        if (res == Z_CHANNEL_NODATA) {
            printf(">> [Subscriber] Nothing to pull... sleep for %zu ms\n", INTERVAL);
            z_sleep_ms(INTERVAL);
        } else {
            break;
        }
    }

    z_subscriber_drop(z_subscriber_move(&sub));
    z_ring_handler_sample_drop(z_ring_handler_sample_move(&handler));

    z_session_drop(z_session_move(&s));
    printf("OK!\n");

    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.\n");
    return -2;
}
#endif
