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
#define URI "/demo/example/zenoh-pico-eval"
#define VALUE "Eval from pico (ESP32)!"

void query_handler(z_query_t *query, const void *arg)
{
    (void)(arg); // Unused paramater
    _z_string_t res = _z_query_res_name(query);
    _z_string_t pred = _z_query_predicate(query);
    _z_send_reply(query, query->rname, (const unsigned char *)VALUE, strlen(VALUE));
}

void setup()
{
    // Set WiFi in STA mode and trigger attachment
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

    // Keep trying until connected
    while (WiFi.status() != WL_CONNECTED)
        delay(1000);

    _z_config_t *config = _z_config_default();
    _z_config_insert(config, Z_CONFIG_MODE_KEY, z_string_make(MODE));
    _z_config_insert(config, Z_CONFIG_PEER_KEY, z_string_make(PEER));

    _z_session_t *s = _z_open(config);
    if (s == NULL)
        return;

    _z_keyexpr_t keyexpr = _z_rname(URI);
    _z_queryable_t *qable = _z_declare_queryable(s, keyexpr, Z_QUERYABLE_EVAL, query_handler, NULL);
    if (qable == 0)
        return;

    _zp_start_read_task(s);
    _zp_start_lease_task(s);
}

void loop()
{
    delay(5000);
}
