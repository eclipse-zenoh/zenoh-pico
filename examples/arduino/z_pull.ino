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

#include <Arduino.h>
#include <WiFi.h>
#include <zenoh-pico.h>

#if Z_FEATURE_SUBSCRIPTION == 1
// WiFi-specific parameters
#define SSID "SSID"
#define PASS "PASS"

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

// @TODO
// z_owned_pull_subscriber_t sub;

// @TODO
// void data_handler(const z_loaned_sample_t *sample, void *arg) {
//     z_view_string_t keystr;
//     z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
//     std::string val((const char *)sample->payload.start, sample->payload.len);

//     Serial.print(" >> [Subscription listener] Received (");
//     Serial.print(z_string_data(z_view_string_loan(&keystr)));
//     Serial.print(", ");
//     Serial.print(val.c_str());
//     Serial.println(")");

// }

void setup() {
    // Initialize Serial for debug
    Serial.begin(115200);
    while (!Serial) {
        delay(1000);
    }

    // Set WiFi in STA mode and trigger attachment
    Serial.print("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    Serial.println("OK");

    // Initialize Zenoh Session and other parameters
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_MODE_KEY, MODE);
    if (strcmp(CONNECT, "") != 0) {
        zp_config_insert(z_config_loan_mut(&config), Z_CONFIG_CONNECT_KEY, CONNECT);
    }

    // Open Zenoh session
    Serial.print("Opening Zenoh Session...");
    z_owned_session_t s;
    if (z_open(&s, z_config_move(&config)) < 0) {
        Serial.println("Unable to open session!");
        while (1) {
            ;
        }
    }
    Serial.println("OK");

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_session_loan_mut(&s), NULL);
    zp_start_lease_task(z_session_loan_mut(&s), NULL);

    // Declare Zenoh subscriber
    Serial.print("Declaring Subscriber on ");
    Serial.print(KEYEXPR);
    Serial.println(" ...");
    // @TODO
    // z_owned_closure_sample_t callback;
    // z_closure_sample(&callback, data_handler, NULL, NULL);
    // @TODO
    // z_view_keyexpr_t ke;
    // z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    // sub = z_declare_pull_subscriber(z_session_loan(&s), z_view_keyexpr_loan(&ke), z_closure_sample_move(&callback),
    // NULL); if (!z_pull_subscriber_check(&sub)) {
    //     Serial.println("Unable to declare subscriber.");
    //     while (1) {
    //         ;
    //     }
    // }
    // Serial.println("OK");
    // Serial.println("Zenoh setup finished!");
    Serial.println("Pull Subscriber not supported... exiting");

    delay(300);
}

void loop() {
    delay(5000);
    // z_subscriber_pull(z_pull_subscriber_loan(&sub));
}
#else
void setup() {
    Serial.println("ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION but this example requires it.");
    return;
}
void loop() {}
#endif
