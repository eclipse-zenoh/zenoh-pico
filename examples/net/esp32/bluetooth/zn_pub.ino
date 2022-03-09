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
#define PEER "bt/esp-pico#mode=slave"
#define URI "/demo/example/switch/led/1"

#define BEEP_PIN 32

zn_session_t *s = NULL;
zn_reskey_t *reskey = NULL;

bool status = false;

void setup()
{
    Serial.begin(115200);

    pinMode(BEEP_PIN, OUTPUT);

    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_MODE_KEY, z_string_make(MODE));
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(PEER));

    Serial.print("Opening Session on classic Bluetooth as slave: ");
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

    reskey = (zn_reskey_t *)malloc(sizeof(zn_reskey_t));
    *reskey = zn_rname(URI);
}

void loop()
{
    delay(5000);

    char *buf = NULL;
    if (status)
        buf = "ON";
    else
        buf = "OFF";
    status = !status;

    digitalWrite(BEEP_PIN, HIGH);
    zn_write(s, *reskey, (const uint8_t *)buf, strlen(buf));
    delay(100);
    digitalWrite(BEEP_PIN, LOW);

    Serial.print("Published ");
    Serial.print(buf);
    Serial.print(" over Bluetooth on ");
    Serial.println(URI);
}
