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

extern "C"
{
#include "zenoh-pico.h"
}

#define SSID "SSID"
#define PASS "PASSWORD"

// Zenoh-specific parameters
#define MODE "client"
#define PEER "tcp/10.0.0.1:7447"
#define URI "/demo/example/**"

zn_subscriber_t *sub = NULL;

void data_handler(const zn_sample_t *sample, const void *arg)
{
    (void)(arg); // Unused argument
}

void setup()
{
    // Set WiFi in STA mode and trigger attachment
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

    // Keep trying until connected
    while (WiFi.status() != WL_CONNECTED)
    {
    }
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

    zn_subinfo_t subinfo;
    subinfo.reliability = zn_reliability_t_RELIABLE;
    subinfo.mode = zn_submode_t_PULL;
    subinfo.period = NULL;
    sub = zn_declare_subscriber(s, zn_rname(URI), subinfo, data_handler, NULL);
    if (sub == NULL)
    {
        return;
    }
}

void loop()
{
    delay(5000);
    if (sub == NULL)
        return;

    zn_pull(sub);
}
