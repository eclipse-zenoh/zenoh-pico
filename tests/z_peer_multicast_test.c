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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/utils/uuid.h"

#undef NDEBUG
#include <assert.h>

#define MSG 10
#define MSG_LEN 1024
#define QRY 10
#define QRY_CLT 10
#define SET 10
#define SLEEP 1
#define TIMEOUT 60

const char *uri = "demo/example/";
unsigned int idx[SET];

// The active subscribers
_z_list_t *subs2 = NULL;

volatile unsigned int total = 0;

volatile unsigned int datas = 0;
void data_handler(z_loaned_sample_t *sample, void *arg) {
    char *res = (char *)malloc(64);
    snprintf(res, 64, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received data: %s\t(%u/%u)\n", res, datas, total);
    z_view_string_t k_str;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &k_str);
    z_owned_slice_t value;
    z_bytes_to_slice(z_sample_payload(sample), &value);
    assert(z_slice_len(z_loan(value)) == MSG_LEN);
    assert(z_string_len(z_loan(k_str)) == strlen(res));
    assert(strncmp(res, z_string_data(z_loan(k_str)), strlen(res)) == 0);
    (void)(sample);

    datas++;
    z_drop(z_move(value));
    free(res);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 1024);

    assert(argc == 2);
    (void)(argc);

    bool is_reliable = strncmp(argv[1], "tcp", 3) == 0;

    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, argv[1]);

    for (unsigned int i = 0; i < SET; i++) idx[i] = i;

    z_owned_session_t s1;
    assert(z_open(&s1, z_move(config), NULL) == Z_OK);
    _z_string_t zid1 = _z_id_to_string(&(_Z_RC_IN_VAL(z_loan(s1))->_local_zid));
    printf("Session 1 with PID: %.*s\n", (int)_z_string_len(&zid1), _z_string_data(&zid1));
    _z_string_clear(&zid1);

    // Start the read session session lease loops
    zp_start_read_task(z_loan_mut(s1), NULL);
    zp_start_lease_task(z_loan_mut(s1), NULL);

    z_sleep_s(SLEEP);

    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, argv[1]);

    z_owned_session_t s2;
    assert(z_open(&s2, z_move(config), NULL) == Z_OK);

    _z_string_t zid2 = _z_id_to_string(&(_Z_RC_IN_VAL(z_loan(s2))->_local_zid));
    printf("Session 2 with PID: %.*s\n", (int)_z_string_len(&zid2), _z_string_data(&zid2));
    _z_string_clear(&zid2);

    // Start the read session session lease loops
    zp_start_read_task(z_loan_mut(s2), NULL);
    zp_start_lease_task(z_loan_mut(s2), NULL);

    z_sleep_s(SLEEP * 5);

    // Declare subscribers on second session
    char *s1_res = (char *)malloc(64);
    for (unsigned int i = 0; i < SET; i++) {
        memset(s1_res, 0, 64);
        snprintf(s1_res, 64, "%s%u", uri, i);
        z_owned_closure_sample_t callback;
        z_closure(&callback, data_handler, NULL, &idx[i]);
        z_owned_subscriber_t *sub = (z_owned_subscriber_t *)z_malloc(sizeof(z_owned_subscriber_t));
        z_view_keyexpr_t ke;
        z_view_keyexpr_from_str(&ke, s1_res);
        z_result_t res = z_declare_subscriber(z_loan(s2), sub, z_loan(ke), z_move(callback), NULL);
        assert(res == _Z_RES_OK);
        printf("Declared subscription on session 2: %ju %i %s\n", (uintmax_t)z_subscriber_loan(sub)->_entity_id, 0,
               s1_res);
        subs2 = _z_list_push(subs2, sub);
    }

    // Write data from first session
    size_t len = MSG_LEN;
    uint8_t *value = (uint8_t *)z_malloc(len);
    memset(value, 1, MSG_LEN);

    total = MSG * SET;
    for (unsigned int n = 0; n < MSG; n++) {
        for (unsigned int i = 0; i < SET; i++) {
            memset(s1_res, 0, 64);
            snprintf(s1_res, 64, "%s%u", uri, i);
            z_put_options_t opt;
            z_put_options_default(&opt);
            opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
            z_view_keyexpr_t ke;
            z_view_keyexpr_from_str(&ke, s1_res);

            // Create payload
            z_owned_bytes_t payload;
            z_bytes_from_buf(&payload, value, len, NULL, NULL);

            z_put(z_loan(s1), z_loan(ke), z_move(payload), &opt);
            printf("Wrote data from session 1: %s %zu b\t(%u/%u)\n", s1_res, len, n * SET + (i + 1), total);
        }
    }

    // Wait to receive all the data
    z_time_t now = z_time_now();
    unsigned int expected = is_reliable ? total : 1;
    while (datas < expected) {
        assert(z_time_elapsed_s(&now) < TIMEOUT);
        (void)(now);
        printf("Waiting for datas... %u/%u\n", datas, expected);
        z_sleep_s(SLEEP);
    }
    if (is_reliable == true)
        assert(datas == expected);
    else
        assert(datas >= expected);
    datas = 0;

    z_sleep_s(SLEEP);

    // Undeclare subscribers and queryables on second session
    while (subs2) {
        z_owned_subscriber_t *sub = _z_list_head(subs2);
        printf("Undeclared subscriber on session 2: %ju\n", (uintmax_t)z_subscriber_loan(sub)->_entity_id);
        z_drop(z_move(*sub));
        subs2 = _z_list_pop(subs2, _z_noop_elem_free, NULL);
    }

    z_sleep_s(SLEEP);

    // Stop read and lease tasks for zenoh-pico
    zp_stop_read_task(z_loan_mut(s1));
    zp_stop_lease_task(z_loan_mut(s1));

    zp_stop_read_task(z_loan_mut(s2));
    zp_stop_lease_task(z_loan_mut(s2));

    // Close both sessions
    printf("Closing session 1\n");
    z_drop(z_move(s1));

    z_sleep_s(SLEEP);

    printf("Closing session 2\n");
    z_drop(z_move(s2));

    z_free((uint8_t *)value);
    value = NULL;

    free(s1_res);

    return 0;
}
