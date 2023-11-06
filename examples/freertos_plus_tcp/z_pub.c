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
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <zenoh-pico.h>

#include "FreeRTOS.h"

#if Z_FEATURE_PUBLICATION == 1
#define CLIENT_OR_PEER 0  // 0: Client mode; 1: Peer mode
#if CLIENT_OR_PEER == 0
#define MODE "client"
#define CONNECT ""  // If empty, it will scout
#elif CLIENT_OR_PEER == 1
#define MODE "peer"
#define CONNECT "udp/224.0.0.225:7447"
#else
#error "Unknown Zenoh operation mode. Check CLIENT_OR_PEER value."
#endif

#define KEYEXPR "demo/example/zenoh-pico-pub"
#define VALUE "[FreeRTOS-Plus-TCP] Pub from Zenoh-Pico!"

void app_main(void) {
    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(MODE));
    if (strcmp(CONNECT, "") != 0) {
        zp_config_insert(z_loan(config), Z_CONFIG_CONNECT_KEY, z_string_make(CONNECT));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return;
    }

    static StackType_t read_task_stack[1000];
    static StaticTask_t read_task_buffer;

    _z_task_attr_t read_task_attr = {
        .name = "ZenohReadTask",
        .priority = 10,
        .stack_depth = 1000,
        .static_allocation = true,
        .stack_buffer = read_task_stack,
        .task_buffer = &read_task_buffer,
    };

    zp_task_read_options_t read_task_opt = {.task_attributes = &read_task_attr};

    static StackType_t lease_task_stack[1000];
    static StaticTask_t lease_task_buffer;

    _z_task_attr_t lease_task_attr = {
        .name = "ZenohLeaseTask",
        .priority = 10,
        .stack_depth = 1000,
        .static_allocation = true,
        .stack_buffer = lease_task_stack,
        .task_buffer = &lease_task_buffer,
    };

    zp_task_lease_options_t lease_task_opt = {.task_attributes = &lease_task_attr};

    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), &read_task_opt) < 0 || zp_start_lease_task(z_loan(s), &lease_task_opt) < 0) {
        printf("Unable to start read and lease tasks\n");
        return;
    }

    printf("Declaring publisher for '%s'...\n", KEYEXPR);
    z_owned_publisher_t pub = z_declare_publisher(z_loan(s), z_keyexpr(KEYEXPR), NULL);
    if (!z_check(pub)) {
        printf("Unable to declare publisher for key expression!\n");
        return;
    }

    char *buf = (char *)pvPortMalloc(256);
    for (int idx = 0; 1; ++idx) {
        z_sleep_s(1);
        snprintf(buf, 256, "[%4d] %s", idx, VALUE);
        printf("Putting Data ('%s': '%s')...\n", KEYEXPR, buf);

        z_publisher_put_options_t options = z_publisher_put_options_default();
        options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
        z_publisher_put(z_loan(pub), (const uint8_t *)buf, strlen(buf), &options);
    }

    z_undeclare_publisher(z_move(pub));

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));

    z_close(z_move(s));
}
#else
void app_main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
}
#endif
