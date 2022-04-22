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
#define URI "/demo/example/zenoh-pico-eval"
#define VALUE "Eval from pico (ESP32)!"

void query_handler(zn_query_t *query, const void *arg)
{
    (void)(arg); // Unused paramater
    z_string_t res = zn_query_res_name(query);
    z_string_t pred = zn_query_predicate(query);
    zn_send_reply(query, query->rname, (const unsigned char *)VALUE, strlen(VALUE));
}

void setup()
{
    // Set WiFi in STA mode and trigger attachment
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

    // Keep trying until connected
    while (WiFi.status() != WL_CONNECTED)
        delay(1000);

    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_MODE_KEY, z_string_make(MODE));
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(PEER));

    zn_session_t *s = zn_open(config);
    if (s == NULL)
        return;

    zn_reskey_t reskey = zn_rname(URI);
    zn_queryable_t *qable = zn_declare_queryable(s, reskey, ZN_QUERYABLE_EVAL, query_handler, NULL);
    if (qable == 0)
        return;

    znp_start_read_task(s);
    znp_start_lease_task(s);
}

void loop()
{
    delay(5000);
}
