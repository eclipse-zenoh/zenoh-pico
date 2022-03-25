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
#include <WiFi.h>

extern "C"
{
#include "zenoh-pico.h"
}

#define SSID "SSID"
#define PASS "PASSWORD"

// Zenoh-specific parameters
#define MODE "client"
#define PEER "tcp/10.0.0.1:7447"
#define URI "/demo/example/zenoh-pico/pub/esp32"

_z_session_t *s = NULL;
_z_reskey_t *reskey = NULL;

void setup()
{
    // Set WiFi in STA mode and trigger attachment
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

    // Keep trying until connected
    while (WiFi.status() != WL_CONNECTED)
        delay(1000);

    _z_properties_t *config = _z_properties_default();
    _z_properties_insert(config, Z_CONFIG_MODE_KEY, z_string_make(MODE));
    _z_properties_insert(config, Z_CONFIG_PEER_KEY, z_string_make(PEER));

    s = _z_open(config);
    if (s == NULL)
        return;

    _zp_start_read_task(s);
    _zp_start_lease_task(s);

    unsigned long rid = _z_declare_resource(s, _z_rname(URI));
    reskey = (_z_reskey_t *)malloc(sizeof(_z_reskey_t));
    *reskey = _z_rid(rid);
}

void loop()
{
    delay(5000);
    if (s == NULL)
        return;

    if (reskey == NULL)
        return;

    char *buf = "Publishing data from ESP32";
    _z_write(s, *reskey, (const uint8_t *)buf, strlen(buf));
}
