/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <Arduino.h>
#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
    #error Bluetooth is not enabled.
#endif

extern "C"
{
    #include "zenoh-pico.h"
}

// Zenoh-specific parameters
#define MODE "peer"
#define PEER "bt/esp-pico#mode=master"
#define URI "/demo/example/switch/led/**"

#define LED_PIN 32

zn_session_t *s = NULL;

void data_handler(const zn_sample_t *sample, const void *arg)
{
    // FIXME: move to non-null terminated strings
    char val[sample->value.len + 1] ;
    memcpy(val, sample->value.val, sample->value.len);
    val[sample->value.len] = '\0';

    // FIXME: move to non-null terminated strings
    char key[sample->key.len + 1];
    memcpy(key, sample->key.val, sample->key.len);
    key[sample->key.len] = '\0';

    Serial.print("Received ");
    Serial.print(val);
    Serial.print(" on ");
    Serial.println(key);

    if (strcmp(val, "ON") == 0)
        digitalWrite(LED_PIN, HIGH);
    else
        digitalWrite(LED_PIN, LOW);
}

void setup()
{
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);

    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_MODE_KEY, z_string_make(MODE));
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(PEER));

    Serial.print("Opening Session on classic Bluetooth as master: ");
    Serial.print(PEER);
    Serial.print("...");

    s = zn_open(config);
    if (s == NULL)
        while(true);
    Serial.println("OK");

    Serial.print("Starting Leasing and Reading tasks...");
    znp_start_read_task(s);
    znp_start_lease_task(s);
    Serial.println("OK");

    Serial.print("Declaring subscriber on: ");
    Serial.print(URI);
    Serial.print("...");
    zn_reskey_t reskey = zn_rname(URI);
    zn_subscriber_t *sub = zn_declare_subscriber(s, reskey, zn_subinfo_default(), data_handler, NULL);
    if (sub == 0)
        while(true);
    Serial.println("OK");
}

void loop()
{
    delay(1000);
}
