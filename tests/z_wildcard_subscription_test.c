//
// Copyright (c) 2025 ZettaScale Technology
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
// Copyright (c) 2022 ZettaScale Technology
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/string.h"

#undef NDEBUG
#include <assert.h>

#define MSGS 10
#define SLEEP 1
#define TIMEOUT 15

static const char *PUB_KE = "test/wildcard";
static const char *SUB1_KE = "*/wildcard";
static const char *SUB2_KE = "**/wildcard";

volatile unsigned int sub1_count = 0;
volatile unsigned int sub2_count = 0;

static void sub_handler_1(z_loaned_sample_t *sample, void *arg) {
    (void)arg;
    z_owned_slice_t value;
    z_bytes_to_slice(z_sample_payload(sample), &value);

    // Both subscribers should see key "test/wildcard"
    z_view_string_t k;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &k);
    // Flawfinder: ignore [CWE-126]
    assert(strncmp(z_string_data(z_loan(k)), PUB_KE, strlen(PUB_KE)) == 0);

    sub1_count++;
    z_drop(z_move(value));
}

static void sub_handler_2(z_loaned_sample_t *sample, void *arg) {
    (void)arg;
    z_owned_slice_t value;
    z_bytes_to_slice(z_sample_payload(sample), &value);

    z_view_string_t k;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &k);
    // Flawfinder: ignore [CWE-126]
    assert(strncmp(z_string_data(z_loan(k)), PUB_KE, strlen(PUB_KE)) == 0);

    sub2_count++;
    z_drop(z_move(value));
}

int main(int argc, char **argv) {
    assert(argc == 2);

    // --- Session 1: publisher ---
    z_owned_config_t cfg1;
    z_config_default(&cfg1);
    zp_config_insert(z_loan_mut(cfg1), Z_CONFIG_CONNECT_KEY, argv[1]);

    z_owned_session_t s_pub;
    assert(z_open(&s_pub, z_move(cfg1), NULL) == Z_OK);
    printf("Publisher session opened\n");
    fflush(stdout);
    assert(zp_start_read_task(z_loan_mut(s_pub), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s_pub), NULL) == Z_OK);

    // --- Session 2: subscribers ---
    z_owned_config_t cfg2;
    z_config_default(&cfg2);
    zp_config_insert(z_loan_mut(cfg2), Z_CONFIG_CONNECT_KEY, argv[1]);

    z_owned_session_t s_sub;
    assert(z_open(&s_sub, z_move(cfg2), NULL) == Z_OK);
    printf("Subscriber session opened\n");
    assert(zp_start_read_task(z_loan_mut(s_sub), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s_sub), NULL) == Z_OK);

    z_sleep_s(SLEEP);

    // Declare publisher keyexpr
    z_view_keyexpr_t pub_ke;
    assert(z_view_keyexpr_from_str(&pub_ke, PUB_KE) == Z_OK);

    // Declare publisher
    z_owned_publisher_t pub;
    assert(z_declare_publisher(z_loan(s_pub), &pub, z_loan(pub_ke), NULL) == Z_OK);
    printf("Declared publisher entity id=%zu\n", z_loan(pub)->_id);
    fflush(stdout);

    // Declare subscribers on separate session
    z_view_keyexpr_t sub1_ke, sub2_ke;
    assert(z_view_keyexpr_from_str(&sub1_ke, SUB1_KE) == Z_OK);
    assert(z_view_keyexpr_from_str(&sub2_ke, SUB2_KE) == Z_OK);

    z_owned_closure_sample_t c1, c2;
    z_closure(&c1, sub_handler_1, NULL, NULL);
    z_closure(&c2, sub_handler_2, NULL, NULL);

    z_owned_subscriber_t sub1, sub2;
    assert(z_declare_subscriber(z_loan(s_sub), &sub1, z_loan(sub1_ke), z_move(c1), NULL) == Z_OK);
    printf("Declared subscriber on '%s'\n", SUB1_KE);
    fflush(stdout);
    assert(z_declare_subscriber(z_loan(s_sub), &sub2, z_loan(sub2_ke), z_move(c2), NULL) == Z_OK);
    printf("Declared subscriber on '%s'\n", SUB2_KE);
    fflush(stdout);

    z_sleep_s(SLEEP);

    // Publish messages
    for (unsigned int i = 0; i < MSGS; i++) {
        char buf[64];
        int n = snprintf(buf, sizeof(buf), "hello-%u", i);

        z_put_options_t opt;
        z_put_options_default(&opt);
        opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;

        z_owned_bytes_t payload;
        z_bytes_from_buf(&payload, (uint8_t *)buf, (size_t)n, NULL, NULL);

        assert(z_put(z_loan(s_pub), z_loan(pub_ke), z_move(payload), &opt) == Z_OK);
        printf("Published %s to %s\n", buf, PUB_KE);
        fflush(stdout);
    }

    // Wait for both subscribers to receive all messages
    z_clock_t start = z_clock_now();
    while ((sub1_count < MSGS || sub2_count < MSGS) && z_clock_elapsed_s(&start) < TIMEOUT) {
        printf("Waiting... sub1=%u/%u sub2=%u/%u\n", sub1_count, MSGS, sub2_count, MSGS);
        fflush(stdout);
        z_sleep_s(SLEEP);
    }

    printf("Final counts: sub1=%u, sub2=%u\n", sub1_count, sub2_count);
    fflush(stdout);
    assert(sub1_count == MSGS);
    assert(sub2_count == MSGS);

    // Cleanup: undeclare
    z_drop(z_move(sub1));
    z_drop(z_move(sub2));
    z_drop(z_move(pub));

    // Stop tasks and close sessions
    zp_stop_read_task(z_loan_mut(s_pub));
    zp_stop_lease_task(z_loan_mut(s_pub));
    zp_stop_read_task(z_loan_mut(s_sub));
    zp_stop_lease_task(z_loan_mut(s_sub));

    z_drop(z_move(s_pub));
    z_drop(z_move(s_sub));

    return 0;
}
