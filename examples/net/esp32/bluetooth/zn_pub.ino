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

_z_session_t *s = NULL;
_z_keyexpr_t *keyexpr = NULL;

bool status = false;

void setup()
{
    Serial.begin(115200);

    pinMode(BEEP_PIN, OUTPUT);

    _z_properties_t *config = _z_properties_default();
    _z_properties_insert(config, Z_CONFIG_MODE_KEY, z_string_make(MODE));
    _z_properties_insert(config, Z_CONFIG_PEER_KEY, z_string_make(PEER));

    Serial.print("Opening Session on classic Bluetooth as slave: ");
    Serial.print(PEER);
    Serial.print("...");

    s = _z_open(config);
    if (s == NULL)
        while(true);
    Serial.println("OK");

    Serial.print("Starting Leasing and Reading tasks...");
    _zp_start_read_task(s);
    _zp_start_lease_task(s);
    Serial.println("OK");

    keyexpr = (_z_keyexpr_t *)malloc(sizeof(_z_keyexpr_t));
    *keyexpr = _z_rname(URI);
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
    _z_write(s, *keyexpr, (const uint8_t *)buf, strlen(buf));
    delay(100);
    digitalWrite(BEEP_PIN, LOW);

    Serial.print("Published ");
    Serial.print(buf);
    Serial.print(" over Bluetooth on ");
    Serial.println(URI);
}
