// /*
//  * Copyright (c) 2017, 2020 ADLINK Technology Inc.
//  *
//  * This program and the accompanying materials are made available under the
//  * terms of the Eclipse Public License 2.0 which is available at
//  * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
//  * which is available at https://www.apache.org/licenses/LICENSE-2.0.
//  *
//  * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//  *
//  * Contributors:
//  *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
//  */

#include <stdio.h>
#include <assert.h>
#include "zenoh-pico/net.h"
#include "zenoh-pico/net/private/system.h"

#define MSG 1000
#define QRY 10
#define SET 1000
#define TIMEOUT 60

char *uri = "/demo/example/";
unsigned int idx[SET];

// The active resource, subscriber, queryable declarations
_z_list_t *pubs1 = NULL;
unsigned long rids1[SET];

_z_list_t *subs2 = NULL;
_z_list_t *qles2 = NULL;
unsigned long rids2[SET];

volatile unsigned int queries = 0;
void query_handler(zn_query_t *query, const void *arg)
{
    queries++;
    char res[64];
    sprintf(res, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received query: %s\t(%u/%u)\n", res, queries, QRY * SET);

    zn_send_reply(query, res, (const uint8_t *)res, strlen(res));
}

volatile unsigned int replies = 0;
void reply_handler(const zn_source_info_t *info, const zn_sample_t *sample, const void *arg)
{
    (void)(info);   // Unused parameter
    (void)(sample); // Unused parameter

    replies++;
    char res[64];
    sprintf(res, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received reply: %s\t(%u/%u)\n", res, replies, QRY * SET);
}

volatile unsigned int datas = 0;
void data_handler(const zn_sample_t *sample, const void *arg)
{
    (void)(sample); // Unused parameter

    datas++;
    char res[64];
    sprintf(res, "%s%u", uri, *(unsigned int *)arg);
    printf(">> Received data: %s\t(%u/%u)\n", res, datas, MSG * SET);
}

int main(void)
{
    setbuf(stdout, NULL);

    for (unsigned int i = 0; i < SET; i++)
        idx[i] = i;

    zn_properties_t *config = zn_config_default();

    zn_session_t *s1 = zn_open(config);
    assert(s1 != NULL);
    z_string_t pid1 = _z_string_from_bytes(&s1->local_pid);
    printf("Session 1 with PID: %s\n", pid1.val);

    // Start the read session session lease loops
    znp_start_read_task(s1);
    znp_start_lease_task(s1);

    zn_session_t *s2 = zn_open(config);
    assert(s2 != NULL);
    z_string_t pid2 = _z_string_from_bytes(&s2->local_pid);
    printf("Session 2 with PID: %s\n", pid2.val);

    // Start the read session session lease loops
    znp_start_read_task(s2);
    znp_start_lease_task(s2);

    // Declare resources on both sessions
    char res[64];
    for (unsigned int i = 0; i < SET; i++)
    {
        sprintf(res, "%s%d", uri, i);
        zn_reskey_t rk = zn_rname(res);
        unsigned long rid = zn_declare_resource(s1, rk);
        printf("Declared resource on session 1: %zu %zu %s\n", rid, rk.rid, rk.rname);
        rids1[i] = rid;
    }

    for (unsigned int i = 0; i < SET; i++)
    {
        sprintf(res, "%s%d", uri, i);
        zn_reskey_t rk = zn_rname(res);
        unsigned long rid = zn_declare_resource(s2, rk);
        printf("Declared resource on session 2: %zu %zu %s\n", rid, rk.rid, rk.rname);
        rids2[i] = rid;
    }

    // Declare subscribers and queryabales on second session
    for (unsigned int i = 0; i < SET; i++)
    {
        zn_reskey_t rk = zn_rid(rids2[i]);
        zn_subscriber_t *sub = zn_declare_subscriber(s2, rk, zn_subinfo_default(), data_handler, &idx[i]);
        assert(sub != NULL);
        printf("Declared subscription on session 2: %zu %zu %s\n", sub->id, rk.rid, rk.rname);
        subs2 = _z_list_cons(subs2, sub);
    }

    for (unsigned int i = 0; i < SET; i++)
    {
        sprintf(res, "%s%d", uri, i);
        zn_reskey_t rk = zn_rname(res);
        zn_queryable_t *qle = zn_declare_queryable(s2, rk, ZN_QUERYABLE_EVAL, query_handler, &idx[i]);
        assert(qle != NULL);
        printf("Declared queryable on session 2: %zu %zu %s\n", qle->id, rk.rid, rk.rname);
        qles2 = _z_list_cons(qles2, qle);
    }

    // Declare publisher on firt session
    for (unsigned int i = 0; i < SET; i++)
    {
        zn_reskey_t rk = zn_rid(rids1[i]);
        zn_publisher_t *pub = zn_declare_publisher(s1, rk);
        assert(pub != NULL);
        printf("Declared publisher on session 1: %zu\n", pub->id);
        pubs1 = _z_list_cons(pubs1, pub);
    }

    // Write data from firt session
    size_t len = 1024;
    const uint8_t *payload = (uint8_t *)malloc(len * sizeof(uint8_t));

    for (unsigned int n = 0; n < MSG; n++)
    {
        for (unsigned int i = 0; i < SET; i++)
        {
            zn_reskey_t rk = zn_rid(rids1[i]);
            zn_write_ext(s1, rk, payload, len, Z_ENCODING_DEFAULT, Z_DATA_KIND_DEFAULT, zn_congestion_control_t_BLOCK);
            printf("Wrote data from session 1: %zu %zu b\t(%u/%u)\n", rk.rid, len, (n + 1) * (i + 1), MSG * SET);
        }
    }

    // Wait to receive all the data
    _zn_clock_t now = _zn_clock_now();
    while (datas < MSG * SET)
    {
        assert(_zn_clock_elapsed_s(&now) < TIMEOUT);
        printf("Waiting for datas... %u/%u\n", datas, MSG * SET);
        _zn_sleep_s(1);
    }
    datas = 0;

    // Query data from first session
    for (unsigned int n = 0; n < QRY; n++)
    {
        for (unsigned int i = 0; i < SET; i++)
        {
            sprintf(res, "%s%d", uri, i);
            zn_reskey_t rk = zn_rname(res);
            zn_query(s1, rk, "", zn_query_target_default(), zn_query_consolidation_default(), reply_handler, &idx[i]);
            printf("Queried data from session 1: %zu %s\n", rk.rid, rk.rname);
        }
    }

    // Wait to receive all the queries
    now = _zn_clock_now();
    while (queries < QRY * SET)
    {
        assert(_zn_clock_elapsed_s(&now) < TIMEOUT);
        printf("Waiting for queries... %u/%u\n", queries, QRY * SET);
        _zn_sleep_s(1);
    }
    queries = 0;

    // Wait to receive all the replies
    now = _zn_clock_now();
    while (replies < QRY * SET)
    {
        assert(_zn_clock_elapsed_s(&now) < TIMEOUT);
        printf("Waiting for replies... %u/%u\n", replies, QRY * SET);
        _zn_sleep_s(1);
    }
    replies = 0;

    // Undeclare publishers on first session
    while (pubs1)
    {
        zn_publisher_t *pub = _z_list_head(pubs1);
        zn_undeclare_publisher(pub);
        printf("Undeclared publisher on session 2: %zu\n", pub->id);
        pubs1 = _z_list_pop(pubs1);
    }

    // Undeclare subscribers and queryables on second session
    while (subs2)
    {
        zn_subscriber_t *sub = _z_list_head(subs2);
        zn_undeclare_subscriber(sub);
        printf("Undeclared subscriber on session 2: %zu\n", sub->id);
        subs2 = _z_list_pop(subs2);
    }

    while (qles2)
    {
        zn_queryable_t *qle = _z_list_head(qles2);
        zn_undeclare_queryable(qle);
        printf("Undeclared queryable on session 2: %zu\n", qle->id);
        qles2 = _z_list_pop(qles2);
    }

    // Undeclare resources on both sessions
    for (unsigned int i = 0; i < SET; i++)
    {
        zn_undeclare_resource(s1, rids1[i]);
        printf("Undeclared resource on session 1: %zu\n", rids1[i]);
    }

    for (unsigned int i = 0; i < SET; i++)
    {
        zn_undeclare_resource(s2, rids2[i]);
        printf("Undeclared resource on session 2: %zu\n", rids2[i]);
    }

    // Stop and close both sessions
    printf("Stopping threads on session 1\n");
    znp_stop_lease_task(s1);
    znp_stop_read_task(s1);

    printf("Stopping threads on session 2\n");
    znp_stop_lease_task(s2);
    znp_stop_read_task(s2);

    printf("Closing session 1\n");
    zn_close(s1);
    printf("Closing session 2\n");
    zn_close(s2);

    zn_properties_free(config);

    return 0;
}
