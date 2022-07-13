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

char *uri = "/demo/example/";
unsigned int idx[SET];

// The active resource, subscriber, queryable declarations
_z_list_t *pubs1 = NULL; // @TODO: use type-safe list
z_owned_keyexpr_t rids1[SET];

_z_list_t *subs2 = NULL; // @TODO: use type-safe list
_z_list_t *qles2 = NULL; // @TODO: use type-safe list
z_owned_keyexpr_t rids2[SET];

volatile unsigned int total = 0;

volatile unsigned int queries = 0;
void query_handler(z_query_t *query, void *arg)
{
    char res[64];
    sprintf(res, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received query: %s\t(%u/%u)\n", res, queries, total);
    assert(_z_str_eq(query->key.suffix, res));
    assert(_z_str_eq(query->predicate, ""));

    z_query_reply(query, z_keyexpr(res), (const uint8_t *)res, strlen(res));
    queries++;
}

volatile unsigned int replies = 0;
void reply_handler(z_owned_reply_t oreply, void *arg)
{
    char res[64];
    sprintf(res, "%s%u", uri, *(unsigned int *)arg);

    z_reply_t *reply = z_loan(oreply);
    if (reply->tag == Z_REPLY_TAG_DATA)
    {
        printf(">> Received reply data: %s %s\t(%u/%u)\n", res, reply->data.sample.key.suffix, replies, total);
        assert(reply->data.sample.value.len == strlen(res));
        assert(strncmp(res, (const char *)reply->data.sample.value.start, strlen(res)) == 0);
        assert(strlen(reply->data.sample.key.suffix) == strlen(res));
        assert(strncmp(res, reply->data.sample.key.suffix, strlen(res)) == 0);
    }
    else
    {
        replies++;
        printf(">> Received reply final: %s\t(%u/%u)\n", res, replies, total);
    }
}

volatile unsigned int datas = 0;
void data_handler(const z_sample_t *sample, void *arg)
{
    char res[64];
    sprintf(res, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received data: %s\t(%u/%u)\n", res, datas, total);

    assert(sample->value.len == MSG_LEN);
    assert(strlen(sample->key.suffix) == strlen(res));
    assert(strncmp(res, sample->key.suffix, strlen(res)) == 0);
    (void) (sample);

    datas++;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    (void) (argc);

    setbuf(stdout, NULL);
    int is_reliable = strncmp(argv[1], "tcp", 3) == 0;

    z_owned_config_t config = zp_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_PEER_KEY, z_string_make(argv[1]));

    for (unsigned int i = 0; i < SET; i++)
        idx[i] = i;

    z_owned_session_t s1 = z_open(z_move(config));
    assert(z_check(s1));
    z_string_t pid1 = _z_string_from_bytes(&z_loan(s1)->_tp_manager->_local_pid);
    printf("Session 1 with PID: %s\n", pid1.val);
    _z_string_clear(&pid1);

    // Start the read session session lease loops
    zp_start_read_task(z_loan(s1));
    zp_start_lease_task(z_loan(s1));

    _z_sleep_s(SLEEP);

    config = zp_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_PEER_KEY, z_string_make(argv[1]));

    z_owned_session_t s2 = z_open(z_move(config));
    assert(z_check(s2));
    z_string_t pid2 = _z_string_from_bytes(&z_loan(s2)->_tp_manager->_local_pid);
    printf("Session 2 with PID: %s\n", pid2.val);
    _z_string_clear(&pid2);

    // Start the read session session lease loops
    zp_start_read_task(z_loan(s2));
    zp_start_lease_task(z_loan(s2));

    _z_sleep_s(SLEEP);

    // Declare resources on both sessions
    char s1_res[64];
    for (unsigned int i = 0; i < SET; i++)
    {
        sprintf(s1_res, "%s%d", uri, i);
        z_owned_keyexpr_t expr = z_declare_keyexpr(z_loan(s1), z_keyexpr(s1_res));
        printf("Declared resource on session 1: %lu %s\n", z_loan(expr).id, z_loan(expr).suffix);
        rids1[i] = expr;
    }

    _z_sleep_s(SLEEP);

    for (unsigned int i = 0; i < SET; i++)
    {
        sprintf(s1_res, "%s%d", uri, i);
        z_owned_keyexpr_t expr = z_declare_keyexpr(z_loan(s2), z_keyexpr(s1_res));
        printf("Declared resource on session 2: %lu %s\n", z_loan(expr).id, z_loan(expr).suffix);
        rids2[i] = expr;
    }

    _z_sleep_s(SLEEP);

    // Declare subscribers and queryabales on second session
    for (unsigned int i = 0; i < SET; i++)
    {
        z_owned_closure_sample_t callback = z_closure(data_handler, NULL, &idx[i]);
        z_owned_subscriber_t *sub = (z_owned_subscriber_t*)malloc(sizeof(z_owned_subscriber_t));
        *sub = z_declare_subscriber(z_loan(s2), z_loan(rids2[i]), &callback, NULL);
        assert(z_check(*sub));
        printf("Declared subscription on session 2: %zu %lu %s\n", z_subscriber_loan(sub)->_id, z_loan(rids2[i]).id, "");
        subs2 = _z_list_push(subs2, sub); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    for (unsigned int i = 0; i < SET; i++)
    {
        sprintf(s1_res, "%s%d", uri, i);
        z_owned_closure_query_t callback = z_closure(query_handler, NULL, &idx[i]);
        z_owned_queryable_t *qle = (z_owned_queryable_t*)malloc(sizeof(z_owned_queryable_t));
        *qle = z_declare_queryable(z_loan(s2), z_keyexpr(s1_res), &callback, NULL);
        assert(z_check(*qle));
        printf("Declared queryable on session 2: %zu %lu %s\n", z_loan(*qle)->_id, (z_zint_t)0, s1_res);
        qles2 = _z_list_push(qles2, qle); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    // Declare publisher on first session
    for (unsigned int i = 0; i < SET; i++)
    {
        z_owned_publisher_t *pub = (z_owned_publisher_t*)malloc(sizeof(z_owned_publisher_t));
        *pub = z_declare_publisher(z_loan(s1), z_loan(rids1[i]), NULL);
        if (!z_check(*pub))
        printf("Declared publisher on session 1: %zu\n", z_loan(*pub)->_id);
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
            z_put_options_t opt = z_put_options_default();
            opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
            z_put(z_loan(s1), z_loan(rids1[i]), (const uint8_t *)payload, len, &opt);
            printf("Wrote data from session 1: %lu %zu b\t(%u/%u)\n", z_loan(rids1[i]).id, len, n * SET + (i + 1), total);
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
    total = QRY * SET;
    for (unsigned int n = 0; n < QRY; n++)
    {
        for (unsigned int i = 0; i < SET; i++)
        {
            sprintf(s1_res, "%s%d", uri, i);
            z_owned_closure_reply_t callback = z_closure(reply_handler, NULL, &idx[i]);
            z_get(z_loan(s1), z_keyexpr(s1_res), "", &callback, NULL);
            printf("Queried data from session 1: %lu %s\n", (z_zint_t)0, s1_res);
        }
    }

    // Wait to receive all the expected queries
    now = _z_clock_now();
    expected = is_reliable ? total : 1;
    while (queries < expected)
    { 
        assert(_z_clock_elapsed_s(&now) < TIMEOUT);
        printf("Waiting for queries... %u/%u\n", queries, expected);
        _z_sleep_s(SLEEP);
    }
    if (is_reliable)
        assert(queries == expected);
    else
        assert(queries >= expected);
    queries = 0;

    // Wait to receive all the expectred replies
    now = _z_clock_now();
    while (replies < expected)
    {
        assert(_z_clock_elapsed_s(&now) < TIMEOUT);
        printf("Waiting for replies... %u/%u\n", replies, expected);
        _z_sleep_s(SLEEP);
    }
    if (is_reliable)
        assert(replies == expected);
    else
        assert(replies >= expected);
    replies = 0;

    _z_sleep_s(SLEEP);

    // Undeclare publishers on first session
    while (pubs1)
    {
        z_owned_publisher_t *pub = _z_list_head(pubs1); // @TODO: use type-safe list
        printf("Undeclared publisher on session 2: %zu\n", z_loan(*pub)->_id);
        z_undeclare_publisher(z_move(*pub));
        pubs1 = _z_list_pop(pubs1, _z_noop_elem_free); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    // Undeclare subscribers and queryables on second session
    while (subs2)
    {
        z_owned_subscriber_t *sub = _z_list_head(subs2); // @TODO: use type-safe list
        printf("Undeclared subscriber on session 2: %zu\n", z_loan(*sub)->_id);
        z_undeclare_subscriber(z_move(*sub));
        subs2 = _z_list_pop(subs2, _z_noop_elem_free); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    while (qles2)
    {
        z_owned_queryable_t *qle = _z_list_head(qles2); // @TODO: use type-safe list
        printf("Undeclared queryable on session 2: %zu\n", z_loan(*qle)->_id);
        z_undeclare_queryable(z_move(*qle));
        qles2 = _z_list_pop(qles2, _z_noop_elem_free); // @TODO: use type-safe list
    }

    _z_sleep_s(SLEEP);

    // Undeclare resources on both sessions
    for (unsigned int i = 0; i < SET; i++)
    {
        printf("Undeclared resource on session 1: %lu\n", z_loan(rids1[i]).id);
        z_undeclare_keyexpr(z_loan(s1), z_move(rids1[i]));
    }

    _z_sleep_s(SLEEP);

    for (unsigned int i = 0; i < SET; i++)
    {
        printf("Undeclared resource on session 2: %lu\n", z_loan(rids2[i]).id);
        z_undeclare_keyexpr(z_loan(s2), z_move(rids2[i]));
    }

    _z_sleep_s(SLEEP);

    // Stop both sessions
    printf("Stopping threads on session 1\n");
    zp_stop_read_task(z_loan(s1));
    zp_stop_lease_task(z_loan(s1));

    printf("Stopping threads on session 2\n");
    zp_stop_read_task(z_loan(s2));
    zp_stop_lease_task(z_loan(s2));

    // Close both sessions
    printf("Closing session 1\n");
    z_close(z_move(s1));

    _z_sleep_s(SLEEP);

    printf("Closing session 2\n");
    z_close(z_move(s2));

    free((uint8_t *)payload);
    payload = NULL;

    return 0;
}
