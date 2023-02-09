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

void fprintzid(z_bytes_t zid) {
    if (zid.start == NULL) {
        Serial.print("None");
    } else {
        Serial.print("Some(");
        for (unsigned int i = 0; i < zid.len; i++) {
            Serial.print(zid.start[i], HEX);
        }
        Serial.print(")");
    }
}

void fprintwhatami(unsigned int whatami) {
    if (whatami == Z_WHATAMI_ROUTER) {
        Serial.print("'Router'");
    } else if (whatami == Z_WHATAMI_PEER) {
        Serial.print("'Peer'");
    } else {
        Serial.print("'Other'");
    }
}

void fprintlocators(const z_str_array_t *locs) {
    Serial.print("[");
    size_t len = z_str_array_len(locs);
    for (unsigned int i = 0; i < len; i++) {
        Serial.print("'");
        Serial.print(*z_str_array_get(locs, i));
        Serial.print("'");
        if (i < len - 1) {
            Serial.print(", ");
        }
    }
    Serial.print("]");
}

void fprinthello(const z_hello_t hello) {
    Serial.print(" >> Hello { zid: ");
    fprintzid(hello.zid);
    Serial.print(", whatami: ");
    fprintwhatami(hello.whatami);
    Serial.print(", locators: ");
    fprintlocators(&hello.locators);
    Serial.println(" }");
}

void callback(z_owned_hello_t *hello, void *context) {
    fprinthello(z_hello_loan(hello));
    Serial.println("");
    (*(int *)context)++;
}

void drop(void *context) {
    int count = *(int *)context;
    free(context);
    if (!count) {
        printf("Did not find any zenoh process.\n");
    } else {
        printf("Dropping scout results.\n");
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
}

void loop() {
    int *context = (int *)malloc(sizeof(int));
    *context = 0;
    z_owned_scouting_config_t config = z_scouting_config_default();
    z_owned_closure_hello_t closure = z_closure_hello(callback, drop, context);
    printf("Scouting...\n");
    z_scout(z_scouting_config_move(&config), z_closure_hello_move(&closure));
}
