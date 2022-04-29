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

#include <stdio.h>
#include <assert.h>
#include "zenoh-pico.h"

#define MSG 1000
#define MSG_LEN 1024
#define QRY 100
#define QRY_CLT 10
#define SET 100
#define SLEEP 1
#define TIMEOUT 60

z_str_t uri = "/demo/example/";
unsigned int idx[SET];

// The active resource, subscriber, queryable declarations
_z_list_t *pubs1 = NULL; // @TODO: use type-safe list
unsigned long rids1[SET];

_z_list_t *subs2 = NULL; // @TODO: use type-safe list
_z_list_t *qles2 = NULL; // @TODO: use type-safe list
unsigned long rids2[SET];

volatile unsigned int total = 0;

volatile unsigned int queries = 0;
void query_handler(const z_query_t *query, const void *arg)
{
    char res[64];
    sprintf(res, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received query: %s\t(%u/%u)\n", res, queries, total);
    assert(_z_str_eq(query->_rname, res));
    assert(_z_str_eq(query->_predicate, ""));

    z_send_reply(query, res, (const uint8_t *)res, strlen(res));

    queries++;
}

volatile unsigned int replies = 0;
void reply_handler(const z_reply_t reply, const void *arg)
{
    char res[64];
    sprintf(res, "%s%u", uri, *(unsigned int *)arg);

    if (reply._tag == Z_REPLY_TAG_DATA)
    {
        printf(">> Received reply data: %s %s\t(%u/%u)\n", res, reply._data._sample._key._rname, replies, total);
        assert(reply._data._sample._value.len == strlen(res));
        assert(strncmp(res, (const z_str_t)reply._data._sample._value.val, strlen(res)) == 0);
        assert(strlen(reply._data._sample._key._rname) == strlen(res));
        assert(strncmp(res, reply._data._sample._key._rname, strlen(res)) == 0);
    }
    else
    {
        replies++;
        printf(">> Received reply final: %s\t(%u/%u)\n", res, replies, total);
    }
}

volatile unsigned int datas = 0;
void data_handler(const z_sample_t *sample, const void *arg)
{
    char res[64];
    sprintf(res, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received data: %s\t(%u/%u)\n", res, datas, total);

    assert(sample->_value.len == MSG_LEN);
    assert(strlen(sample->_key._rname) == strlen(res));
    assert(strncmp(res, sample->_key._rname, strlen(res)) == 0);
    (void) (sample);

    datas++;
}

int main(int argc, z_str_t *argv)
{
    assert(argc == 2);
    (void) (argc);

    setbuf(stdout, NULL);
    int is_reliable = strncmp(argv[1], "tcp", 3) == 0;

    z_owned_config_t config = z_config_default();
    z_config_insert(z_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(argv[1]));

    for (unsigned int i = 0; i < SET; i++)
        idx[i] = i;

    z_owned_session_t s1 = z_open(z_move(&config));
    assert(z_check(&s1));
    z_string_t pid1 = _z_string_from_bytes(&z_loan(&s1)->_tp_manager->_local_pid);
    printf("Session 1 with PID: %s\n", pid1.val);
    _z_string_clear(&pid1);

    // Start the read session session lease loops
    _zp_start_read_task(z_loan(&s1));
    _zp_start_lease_task(z_loan(&s1));

    _z_sleep_s(SLEEP);

    config = z_config_default();
    z_config_insert(z_loan(&config), Z_CONFIG_PEER_KEY, z_string_make(argv[1]));

    z_owned_session_t s2 = z_open(z_move(&config));
    assert(z_check(&s2));
    z_string_t pid2 = _z_string_from_bytes(&z_loan(&s2)->_tp_manager->_local_pid);
    printf("Session 2 with PID: %s\n", pid2.val);
    _z_string_clear(&pid2);

    // Start the read session session lease loops
    _zp_start_read_task(z_loan(&s2));
    _zp_start_lease_task(z_loan(&s2));

    _z_sleep_s(SLEEP);

    // Declare resources on both sessions
    char s1_res[64];
    for (unsigned int i = 0; i < SET; i++)
    {
        sprintf(s1_res, "%s%d", uri, i);
        z_owned_keyexpr_t expr = z_declare_expr(z_loan(&s1), z_expr_new(s1_res));
        printf("Declared resource on session 1: %lu %s\n", z_keyexpr_loan(&expr)->_rid, z_keyexpr_loan(&expr)->_rname);
        rids1[i] = z_keyexpr_loan(&expr)->_rid;
        z_keyexpr_clear(z_move(&expr));
    }

    _z_sleep_s(SLEEP);

    for (unsigned int i = 0; i < SET; i++)
    {
        sprintf(s1_res, "%s%d", uri, i);
        z_owned_keyexpr_t expr = z_declare_expr(z_loan(&s2), z_expr_new(s1_res));
        printf("Declared resource on session 2: %lu %s\n", z_keyexpr_loan(&expr)->_rid, z_keyexpr_loan(&expr)->_rname);
        rids2[i] = z_keyexpr_loan(&expr)->_rid;
        z_keyexpr_clear(z_move(&expr));
    }

    _z_sleep_s(SLEEP);

    // Declare subscribers and queryabales on second session
    for (unsigned int i = 0; i < SET; i++)
    {
        z_owned_subscriber_t *sub = (z_owned_subscriber_t*)malloc(sizeof(z_owned_subscriber_t));
        *sub = z_subscribe(z_loan(&s2), z_id_new(rids2[i]), z_subinfo_default(), data_handler, &idx[i]);
        assert(z_check(sub));
        printf("Declared subscription on session 2: %zu %lu %s\n", z_subscriber_loan(sub)->_id, rids2[i], "");
        subs2 = _z_list_push(subs2, sub); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    for (unsigned int i = 0; i < SET; i++)
    {
        sprintf(s1_res, "%s%d", uri, i);
        z_owned_queryable_t *qle = (z_owned_queryable_t*)malloc(sizeof(z_owned_queryable_t));
        *qle = z_queryable_new(z_loan(&s2), z_expr_new(s1_res), Z_QUERYABLE_EVAL, query_handler, &idx[i]);
        assert(z_check(qle));
        printf("Declared queryable on session 2: %zu %lu %s\n", z_queryable_loan(qle)->_id, (z_zint_t)0, s1_res);
        qles2 = _z_list_push(qles2, qle); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    // Declare publisher on first session
    for (unsigned int i = 0; i < SET; i++)
    {
        z_owned_publisher_t *pub = (z_owned_publisher_t*)malloc(sizeof(z_owned_publisher_t));
        *pub = z_declare_publication(z_loan(&s1), z_id_new(rids1[i]));
        if (!z_check(pub))
        printf("Declared publisher on session 1: %zu\n", z_publisher_loan(pub)->_id);
        pubs1 = _z_list_push(pubs1, pub); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    // Write data from firt session
    size_t len = MSG_LEN;
    const uint8_t *payload = (uint8_t *)malloc(len * sizeof(uint8_t));
    memset((uint8_t *)payload, 1, MSG_LEN);

    total = MSG * SET;
    for (unsigned int n = 0; n < MSG; n++)
    {
        for (unsigned int i = 0; i < SET; i++)
        {
            z_owned_keyexpr_t keyexpr = z_id_with_suffix_new(rids1[i], "");
            z_put_options_t opt = z_put_options_default();
            opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
            z_put_ext(z_loan(&s1), z_loan(&keyexpr), (const uint8_t *)payload, len, &opt);
            printf("Wrote data from session 1: %lu %zu b\t(%u/%u)\n", rids1[i], len, n * SET + (i + 1), total);
            z_keyexpr_clear(z_move(&keyexpr));
        }
    }

    // Wait to receive all the data
    _z_clock_t now = _z_clock_now();
    unsigned int expected = is_reliable ? total : 1;
    while (datas < expected)
    {
        assert(_z_clock_elapsed_s(&now) < TIMEOUT);
        printf("Waiting for datas... %u/%u\n", datas, expected);
        _z_sleep_s(SLEEP);
    }
    if (is_reliable)
        assert(datas == expected);
    else
        assert(datas >= expected);
    datas = 0;

    _z_sleep_s(SLEEP);

    // Query data from first session
    // TODO: update when query with callbacks are implemented

    if (is_reliable)
    {
        _z_sleep_s(SLEEP);

        // Query and collect from first session
        total = QRY_CLT * SET;
        for (unsigned int n = 0; n < QRY_CLT; n++)
        {
            for (unsigned int i = 0; i < SET; i++)
            {
                sprintf(s1_res, "%s%d", uri, i);
                z_owned_reply_data_array_t reply_data_a = z_get_collect(z_loan(&s1), z_expr_new(s1_res), "", z_query_target_default(), z_query_consolidation_default());
                printf("Queried and collected data from session 1: %lu %s\n", (z_zint_t)0, s1_res);
                replies += z_reply_data_array_loan(&reply_data_a)->_len;
                z_reply_data_array_clear(z_move(&reply_data_a));
            }
        }
        assert(replies == total);
    }

    _z_sleep_s(SLEEP);

    // Undeclare publishers on first session
    while (pubs1)
    {
        z_owned_publisher_t *pub = _z_list_head(pubs1); // @TODO: use type-safe list
        printf("Undeclared publisher on session 2: %zu\n", z_publisher_loan(pub)->_id);
        z_publisher_close(z_move(pub));
        pubs1 = _z_list_pop(pubs1, _z_noop_elem_free); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    // Undeclare subscribers and queryables on second session
    while (subs2)
    {
        z_owned_subscriber_t *sub = _z_list_head(subs2); // @TODO: use type-safe list
        printf("Undeclared subscriber on session 2: %zu\n", z_subscriber_loan(sub)->_id);
        z_subscriber_close(z_move(sub));
        subs2 = _z_list_pop(subs2, _z_noop_elem_free); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    while (qles2)
    {
        z_owned_queryable_t *qle = _z_list_head(qles2); // @TODO: use type-safe list
        printf("Undeclared queryable on session 2: %zu\n", z_queryable_loan(qle)->_id);
        z_queryable_close(z_move(qle));
        qles2 = _z_list_pop(qles2, _z_noop_elem_free); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    // Undeclare resources on both sessions
    for (unsigned int i = 0; i < SET; i++)
    {
        printf("Undeclared resource on session 1: %lu\n", rids1[i]);
        z_undeclare_expr(z_loan(&s1), z_id_new(rids1[i]));
    }

    _z_sleep_s(SLEEP);

    for (unsigned int i = 0; i < SET; i++)
    {
        printf("Undeclared resource on session 2: %lu\n", rids2[i]);
        z_undeclare_expr(z_loan(&s2), z_id_new(rids2[i]));
    }

    _z_sleep_s(SLEEP);

    // Stop both sessions
    printf("Stopping threads on session 1\n");
    _zp_stop_read_task(z_loan(&s1));
    _zp_stop_lease_task(z_loan(&s1));

    printf("Stopping threads on session 2\n");
    _zp_stop_read_task(z_loan(&s2));
    _zp_stop_lease_task(z_loan(&s2));

    // Close both sessions
    printf("Closing session 1\n");
    z_close(z_move(&s1));

    _z_sleep_s(SLEEP);

    printf("Closing session 2\n");
    z_close(z_move(&s2));

    free((uint8_t *)payload);
    payload = NULL;

    return 0;
}
