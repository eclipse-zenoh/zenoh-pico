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

extern "C" {
    #include "zenoh-pico.h"
}

#define SSID "SSID"
#define PASS "PASSWORD"

// Zenoh-specific parameters
#define MODE "client"
#define PEER "tcp/10.0.0.1:7447"
#define URI "/demo/example/zenoh-pico-esp32/**"

void data_handler(const zn_sample_t *sample, const void *arg)
{
    (void)(arg); // Unused argument

//    printf(">> [Subscription listener] Received (%.*s, %.*s)\n",
//           (int)sample->key.len, sample->key.val,
//           (int)sample->value.len, sample->value.val);
}

void setup()
{
    // Set WiFi in STA mode and trigger attachment
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

    // Keep trying until connected
    while (WiFi.status() != WL_CONNECTED)
    { }
    delay(1000);

    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_MODE_KEY, z_string_make(MODE));
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(PEER));

    zn_session_t *s = zn_open(config);
    if (s == NULL)
    {
        return;
    }

    znp_start_read_task(s);
    znp_start_lease_task(s);

    zn_subscriber_t *sub = zn_declare_subscriber(s, zn_rname(URI), zn_subinfo_default(), data_handler, NULL);
    if (sub == 0)
    {
        return;
    }
}

void loop()
{
    delay(5000);
}
