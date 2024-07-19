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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/string.h"

#undef NDEBUG
#include <assert.h>

#define MSG 1000
#define MSG_LEN 1024
#define FRAGMENT_MSG_NB 100
#define FRAGMENT_MSG_LEN 100000
#define QRY 100
#define QRY_CLT 10
#define SET 100
#define SLEEP 1
#define TIMEOUT 60

const char *uri = "demo/example/";
unsigned int idx[SET];

// The active resource, subscriber, queryable declarations
_z_list_t *pubs1 = NULL;
z_owned_keyexpr_t rids1[SET];

_z_list_t *subs2 = NULL;
_z_list_t *qles2 = NULL;
z_owned_keyexpr_t rids2[SET];

volatile unsigned int total = 0;

volatile unsigned int queries = 0;
void query_handler(const z_loaned_query_t *query, void *arg) {
    char *res = (char *)malloc(64);
    snprintf(res, 64, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received query: %s\t(%u/%u)\n", res, queries, total);

    z_view_string_t k_str;
    z_keyexpr_as_view_string(z_query_keyexpr(query), &k_str);
    assert(_z_str_eq(z_loan(k_str)->val, res) == true);

    z_view_string_t pred;
    z_query_parameters(query, &pred);
    assert(z_loan(pred)->len == strlen(""));
    assert(strncmp((const char *)z_loan(pred)->val, "", strlen("")) == 0);

    // Reply value encoding
    z_owned_bytes_t reply_payload;
    z_bytes_serialize_from_str(&reply_payload, res);

    z_query_reply(query, z_query_keyexpr(query), z_move(reply_payload), NULL);
    queries++;
    free(res);
}

volatile unsigned int replies = 0;
void reply_handler(const z_loaned_reply_t *reply, void *arg) {
    char *res = (char *)malloc(64);
    snprintf(res, 64, "%s%u", uri, *(unsigned int *)arg);
    if (z_reply_is_ok(reply)) {
        const z_loaned_sample_t *sample = z_reply_ok(reply);
        printf(">> Received reply data: %s\t(%u/%u)\n", res, replies, total);

        z_view_string_t k_str;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &k_str);
        z_owned_string_t value;
        z_bytes_deserialize_into_string(z_sample_payload(sample), &value);
        assert(z_string_len(z_loan(value)) == strlen(res));
        assert(strncmp(res, z_string_data(z_loan(value)), strlen(res)) == 0);
        assert(_z_str_eq(z_loan(k_str)->val, res) == true);

        replies++;
        z_drop(z_move(value));
    } else {
        printf(">> Received an error\n");
    }
    free(res);
}

volatile unsigned int datas = 0;
void data_handler(const z_loaned_sample_t *sample, void *arg) {
    char *res = (char *)malloc(64);
    snprintf(res, 64, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received data: %s\t(%u/%u)\n", res, datas, total);

    z_view_string_t k_str;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &k_str);
    z_owned_slice_t value;
    z_bytes_deserialize_into_slice(z_sample_payload(sample), &value);
    size_t payload_len = z_slice_len(z_loan(value));
    assert((payload_len == MSG_LEN) || (payload_len == FRAGMENT_MSG_LEN));
    assert(_z_str_eq(z_loan(k_str)->val, res) == true);

    datas++;
    z_drop(z_move(value));
    free(res);
}

_z_string_t format_id(const z_id_t *id) {
    _z_slice_t id_as_bytes = _z_slice_wrap(id->id, _z_id_len(*id));
    return _z_string_convert_bytes(&id_as_bytes);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 1024);

    assert(argc == 2);
    (void)(argc);

    int is_reliable = strncmp(argv[1], "tcp", 3) == 0;

    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, argv[1]);

    for (unsigned int i = 0; i < SET; i++) idx[i] = i;

    z_owned_session_t s1;
    z_open(&s1, z_move(config));
    assert(z_check(s1));
    _z_string_t zid1 = format_id(&(_Z_RC_IN_VAL(z_loan(s1))->_local_zid));
    printf("Session 1 with PID: %s\n", zid1.val);
    _z_string_clear(&zid1);

    // Start the read session session lease loops
    zp_start_read_task(z_loan_mut(s1), NULL);
    zp_start_lease_task(z_loan_mut(s1), NULL);

    z_sleep_s(SLEEP);

    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, argv[1]);

    z_owned_session_t s2;
    z_open(&s2, z_move(config));
    assert(z_check(s2));
    _z_string_t zid2 = format_id(&(_Z_RC_IN_VAL(z_loan(s2))->_local_zid));
    printf("Session 2 with PID: %s\n", zid2.val);
    _z_string_clear(&zid2);

    // Start the read session session lease loops
    zp_start_read_task(z_loan_mut(s2), NULL);
    zp_start_lease_task(z_loan_mut(s2), NULL);

    z_sleep_s(SLEEP);

    // Declare resources on both sessions
    char *s1_res = (char *)malloc(64);
    for (unsigned int i = 0; i < SET; i++) {
        snprintf(s1_res, 64, "%s%u", uri, i);
        z_view_keyexpr_t ke;
        z_view_keyexpr_from_str(&ke, s1_res);
        z_owned_keyexpr_t expr;
        z_declare_keyexpr(&expr, z_loan(s1), z_loan(ke));
        printf("Declared resource on session 1: %u %s\n", z_loan(expr)->_id, z_loan(expr)->_suffix);
        rids1[i] = expr;
    }

    z_sleep_s(SLEEP);

    for (unsigned int i = 0; i < SET; i++) {
        snprintf(s1_res, 64, "%s%u", uri, i);
        z_view_keyexpr_t ke;
        z_view_keyexpr_from_str(&ke, s1_res);
        z_owned_keyexpr_t expr;
        z_declare_keyexpr(&expr, z_loan(s2), z_loan(ke));
        printf("Declared resource on session 2: %u %s\n", z_loan(expr)->_id, z_loan(expr)->_suffix);
        rids2[i] = expr;
    }

    z_sleep_s(SLEEP);

    // Declare subscribers and queryabales on second session
    for (unsigned int i = 0; i < SET; i++) {
        z_owned_closure_sample_t callback;
        z_closure(&callback, data_handler, NULL, &idx[i]);
        z_owned_subscriber_t *sub = (z_owned_subscriber_t *)z_malloc(sizeof(z_owned_subscriber_t));
        int8_t res = z_declare_subscriber(sub, z_loan(s2), z_loan(rids2[i]), &callback, NULL);
        assert(res == _Z_RES_OK);
        printf("Declared subscription on session 2: %ju %u %s\n", (uintmax_t)z_subscriber_loan(sub)->_entity_id,
               z_loan(rids2[i])->_id, "");
        subs2 = _z_list_push(subs2, sub);
    }

    z_sleep_s(SLEEP);

    for (unsigned int i = 0; i < SET; i++) {
        snprintf(s1_res, 64, "%s%u", uri, i);
        z_owned_closure_query_t callback;
        z_closure(&callback, query_handler, NULL, &idx[i]);
        z_owned_queryable_t *qle = (z_owned_queryable_t *)z_malloc(sizeof(z_owned_queryable_t));
        z_view_keyexpr_t ke;
        z_view_keyexpr_from_str(&ke, s1_res);
        assert(z_declare_queryable(qle, z_loan(s2), z_loan(ke), &callback, NULL) == _Z_RES_OK);
        printf("Declared queryable on session 2: %ju %zu %s\n", (uintmax_t)qle->_val._entity_id, (z_zint_t)0, s1_res);
        qles2 = _z_list_push(qles2, qle);
    }

    z_sleep_s(SLEEP);

    // Declare publisher on first session
    for (unsigned int i = 0; i < SET; i++) {
        z_owned_publisher_t *pub = (z_owned_publisher_t *)z_malloc(sizeof(z_owned_publisher_t));
        if (z_declare_publisher(pub, z_loan(s1), z_loan(rids1[i]), NULL) < 0) {
            printf("Declared publisher on session 1: %zu\n", z_loan(*pub)->_id);
        }
        pubs1 = _z_list_push(pubs1, pub);
    }

    z_sleep_s(SLEEP);

    // Write data from first session
    size_t len = MSG_LEN;
    uint8_t *value = (uint8_t *)z_malloc(len);
    memset(value, 1, MSG_LEN);

    total = MSG * SET;
    for (unsigned int n = 0; n < MSG; n++) {
        for (unsigned int i = 0; i < SET; i++) {
            z_put_options_t opt;
            z_put_options_default(&opt);
            opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;

            // Create payload
            z_owned_bytes_t payload;
            z_bytes_serialize_from_slice(&payload, value, len);

            z_put(z_loan(s1), z_loan(rids1[i]), z_move(payload), &opt);
            printf("Wrote data from session 1: %u %zu b\t(%u/%u)\n", z_loan(rids1[i])->_id, len, n * SET + (i + 1),
                   total);
        }
    }

    // Wait to receive all the data
    z_clock_t now = z_clock_now();
    unsigned int expected = is_reliable ? total : 1;
    while (datas < expected) {
        assert(z_clock_elapsed_s(&now) < TIMEOUT);
        printf("Waiting for datas... %u/%u\n", datas, expected);
        z_sleep_s(SLEEP);
    }
    if (is_reliable == true)
        assert(datas == expected);
    else
        assert(datas >= expected);
    datas = 0;

    z_sleep_s(SLEEP);

    // Write fragment data from first session
    if (is_reliable) {
        z_free((uint8_t *)value);
        len = FRAGMENT_MSG_LEN;
        value = (uint8_t *)z_malloc(len);
        memset(value, 1, FRAGMENT_MSG_LEN);

        total = FRAGMENT_MSG_NB * SET;
        for (unsigned int n = 0; n < FRAGMENT_MSG_NB; n++) {
            for (unsigned int i = 0; i < SET; i++) {
                z_put_options_t opt;
                z_put_options_default(&opt);
                opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;

                // Create payload
                z_owned_bytes_t payload;
                z_bytes_serialize_from_slice(&payload, value, len);

                z_put(z_loan(s1), z_loan(rids1[i]), z_move(payload), &opt);
                printf("Wrote fragment data from session 1: %u %zu b\t(%u/%u)\n", z_loan(rids1[i])->_id, len,
                       n * SET + (i + 1), total);
            }
        }
        // Wait to receive all the data
        now = z_clock_now();
        while (datas < total) {
            assert(z_clock_elapsed_s(&now) < TIMEOUT);
            printf("Waiting for fragment datas... %u/%u\n", datas, total);
            z_sleep_s(SLEEP);
        }
        if (is_reliable == true) {
            assert(datas == total);
        }
        datas = 0;
        z_sleep_s(SLEEP);
    }

    // Query data from first session
    total = QRY * SET;
    for (unsigned int n = 0; n < QRY; n++) {
        for (unsigned int i = 0; i < SET; i++) {
            snprintf(s1_res, 64, "%s%u", uri, i);
            z_owned_closure_reply_t callback;
            z_closure(&callback, reply_handler, NULL, &idx[i]);
            z_view_keyexpr_t ke;
            z_view_keyexpr_from_str(&ke, s1_res);
            z_get(z_loan(s1), z_loan(ke), "", &callback, NULL);
            printf("Queried data from session 1: %zu %s\n", (z_zint_t)0, s1_res);
        }
    }

    // Wait to receive all the expected queries
    now = z_clock_now();
    expected = is_reliable ? total : 1;
    while (queries < expected) {
        assert(z_clock_elapsed_s(&now) < TIMEOUT);
        printf("Waiting for queries... %u/%u\n", queries, expected);
        z_sleep_s(SLEEP);
    }
    if (is_reliable == true)
        assert(queries == expected);
    else
        assert(queries >= expected);
    queries = 0;

    // Wait to receive all the expected replies
    now = z_clock_now();
    while (replies < expected) {
        assert(z_clock_elapsed_s(&now) < TIMEOUT);
        printf("Waiting for replies... %u/%u\n", replies, expected);
        z_sleep_s(SLEEP);
    }
    if (is_reliable == true)
        assert(replies == expected);
    else
        assert(replies >= expected);
    replies = 0;

    z_sleep_s(SLEEP);

    // Undeclare publishers on first session
    while (pubs1) {
        z_owned_publisher_t *pub = _z_list_head(pubs1);
        printf("Undeclared publisher on session 2: %zu\n", z_loan(*pub)->_id);
        z_undeclare_publisher(z_move(*pub));
        pubs1 = _z_list_pop(pubs1, _z_noop_elem_free, NULL);
    }

    z_sleep_s(SLEEP);

    // Undeclare subscribers and queryables on second session
    while (subs2) {
        z_owned_subscriber_t *sub = _z_list_head(subs2);
        printf("Undeclared subscriber on session 2: %ju\n", (uintmax_t)sub->_val._entity_id);
        z_undeclare_subscriber(z_move(*sub));
        subs2 = _z_list_pop(subs2, _z_noop_elem_free, NULL);
    }

    z_sleep_s(SLEEP);

    while (qles2) {
        z_owned_queryable_t *qle = _z_list_head(qles2);
        printf("Undeclared queryable on session 2: %ju\n", (uintmax_t)qle->_val._entity_id);
        z_undeclare_queryable(z_move(*qle));
        qles2 = _z_list_pop(qles2, _z_noop_elem_free, NULL);
    }

    z_sleep_s(SLEEP);

    // Undeclare resources on both sessions
    for (unsigned int i = 0; i < SET; i++) {
        printf("Undeclared resource on session 1: %u\n", z_loan(rids1[i])->_id);
        z_undeclare_keyexpr(z_move(rids1[i]), z_loan(s1));
    }

    z_sleep_s(SLEEP);

    for (unsigned int i = 0; i < SET; i++) {
        printf("Undeclared resource on session 2: %u\n", z_loan(rids2[i])->_id);
        z_undeclare_keyexpr(z_move(rids2[i]), z_loan(s2));
    }

    z_sleep_s(SLEEP);

    // Close both sessions
    printf("Closing session 1\n");
    z_close(z_move(s1));

    z_sleep_s(SLEEP);

    printf("Closing session 2\n");
    z_close(z_move(s2));

    z_free((uint8_t *)value);
    value = NULL;

    free(s1_res);

    return 0;
}
