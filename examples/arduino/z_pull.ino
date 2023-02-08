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

// WiFi-specific parameters
#define SSID "SSID"
#define PASS "PASS"

#define CLIENT_OR_PEER 0  // 0: Client mode; 1: Peer mode
#if CLIENT_OR_PEER == 0
#define MODE "client"
#define PEER ""  // If empty, it will scout
#elif CLIENT_OR_PEER == 1
#define MODE "peer"
#define PEER "udp/224.0.0.225:7447#iface=en0"
#else
#error "Unknown Zenoh operation mode. Check CLIENT_OR_PEER value."
#endif

#define KEYEXPR "demo/example/**"

z_owned_pull_subscriber_t sub;

void data_handler(const z_sample_t *sample, void *arg) {
    z_owned_str_t keystr = z_keyexpr_to_string(sample->keyexpr);
    std::string val((const char *)sample->payload.start, sample->payload.len);

    Serial.print(" >> [Subscription listener] Received (");
    Serial.print(z_str_loan(&keystr));
    Serial.print(", ");
    Serial.print(val.c_str());
    Serial.println(")");

    z_str_drop(z_str_move(&keystr));
}

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
    z_owned_config_t config = z_config_default();
    zp_config_insert(z_config_loan(&config), Z_CONFIG_MODE_KEY, z_string_make(MODE));
    if (strcmp(PEER, "") != 0) {
        zp_config_insert(z_config_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(PEER));
    }

    // Open Zenoh session
    Serial.print("Opening Zenoh Session...");
    z_owned_session_t s = z_open(z_config_move(&config));
    if (!z_session_check(&s)) {
        Serial.println("Unable to open session!");
        while (1) {
            ;
        }
    }
    Serial.println("OK");

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_session_loan(&s), NULL);
    zp_start_lease_task(z_session_loan(&s), NULL);

    // Declare Zenoh subscriber
    Serial.print("Declaring Subscriber on ");
    Serial.print(KEYEXPR);
    Serial.println(" ...");
    z_owned_closure_sample_t callback = z_closure_sample(data_handler, NULL, NULL);
    sub = z_declare_pull_subscriber(z_session_loan(&s), z_keyexpr(KEYEXPR), z_closure_sample_move(&callback), NULL);
    if (!z_pull_subscriber_check(&sub)) {
        Serial.println("Unable to declare subscriber.");
        while (1) {
            ;
        }
    }
    Serial.println("OK");
    Serial.println("Zenoh setup finished!");

    delay(300);
}

void loop() {
    delay(5000);
    z_subscriber_pull(z_pull_subscriber_loan(&sub));
}
