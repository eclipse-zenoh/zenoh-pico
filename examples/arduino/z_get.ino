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

#if Z_FEATURE_QUERY == 1
// WiFi-specific parameters
#define SSID "SSID"
#define PASS "PASS"

// Client mode values (comment/uncomment as needed)
#define MODE "client"
#define LOCATOR ""  // If empty, it will scout
// Peer mode values (comment/uncomment as needed)
// #define MODE "peer"
// #define LOCATOR "udp/224.0.0.225:7447#iface=en0"

#define KEYEXPR "demo/example/**"
#define VALUE ""

z_owned_session_t s;

void reply_dropper(void *ctx) {
    (void)(ctx);
    Serial.println(" >> Received query final notification");
}

void reply_handler(z_loaned_reply_t *oreply, void *ctx) {
    (void)(ctx);
    if (z_reply_is_ok(oreply)) {
        const z_loaned_sample_t *sample = z_reply_ok(oreply);
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t replystr;
        z_bytes_to_string(z_sample_payload(sample), &replystr);

        Serial.print(" >> [Get listener] Received (");
        Serial.write(z_string_data(z_view_string_loan(&keystr)), z_string_len(z_view_string_loan(&keystr)));
        Serial.print(", ");
        Serial.write(z_string_data(z_string_loan(&replystr)), z_string_len(z_string_loan(&replystr)));
        Serial.println(")");

        z_string_drop(z_string_move(&replystr));
    } else {
        Serial.println(" >> Received an error");
    }
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
    Serial.print("Opening Zenoh Session...");
    if (z_open(&s, z_config_move(&config), NULL) < 0) {
        Serial.println("Unable to open session!");
        while (1) {
            ;
        }
    }
    Serial.println("OK");

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_session_loan_mut(&s), NULL) < 0 || zp_start_lease_task(z_session_loan_mut(&s), NULL) < 0) {
        Serial.println("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        while (1) {
            ;
        }
    }
    Serial.println("Zenoh setup finished!");

    delay(300);
}

void loop() {
    delay(5000);

    Serial.print("Sending Query ");
    Serial.print(KEYEXPR);
    Serial.println(" ...");
    z_get_options_t opts;
    z_get_options_default(&opts);
    // Value encoding
    z_owned_bytes_t payload;
    if (strcmp(VALUE, "") != 0) {
        z_bytes_from_static_str(&payload, VALUE);
        opts.payload = z_bytes_move(&payload);
    }
    z_owned_closure_reply_t callback;
    z_closure_reply(&callback, reply_handler, reply_dropper, NULL);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
    if (z_get(z_session_loan(&s), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&callback), &opts) < 0) {
        Serial.println("Unable to send query.");
    }
}
#else
void setup() {
    Serial.println("ERROR: Zenoh pico was compiled without Z_FEATURE_QUERY but this example requires it.");
    return;
}
void loop() {}
#endif
