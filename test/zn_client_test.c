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

#define RUN 1000

void query_handler(zn_query_t *query, const void *arg)
{
    (void)(query); // Unused paramater
    (void)(arg);   // Unused paramater
}

void reply_handler(const zn_source_info_t *info, const zn_sample_t *sample, const void *arg)
{
    (void)(info);   // Unused parameter
    (void)(sample); // Unused parameter
    (void)(arg);    // Unused parameter
}

void data_handler(const zn_sample_t *sample, const void *arg)
{
    (void)(sample); // Unused parameter
    (void)(arg);    // Unused argument
}

int main(void)
{
    char *uri = "/demo/example/";
    zn_properties_t *config = zn_config_default();

    zn_session_t *s1 = zn_open(config);
    assert(s1 != NULL);

    // Start the read session session lease loops
    znp_start_read_task(s1);
    znp_start_lease_task(s1);

    zn_session_t *s2 = zn_open(config);
    assert(s2 != NULL);

    // Start the read session session lease loops
    znp_start_read_task(s2);
    znp_start_lease_task(s2);

    // The active resource, subscriber, queryable declarations
    _z_list_t *pubs1 = _z_list_empty;
    unsigned long rids1[RUN];

    _z_list_t *subs2 = _z_list_empty;
    _z_list_t *qles2 = _z_list_empty;
    unsigned long rids2[RUN];

    // Declare resources on both sessions
    char res[64];
    for (unsigned int i = 0; i < RUN; i++)
    {
        sprintf(res, "%s%d", uri, i);
        zn_reskey_t rk = zn_rname(res);
        unsigned long rid = zn_declare_resource(s1, rk);
        printf("Declared resource on session 1: %zu %s\n", rid, res);
        rids1[i] = rid;
        free(rk.rname);
    }

    for (unsigned int i = 0; i < RUN; i++)
    {
        sprintf(res, "%s%d", uri, i);
        zn_reskey_t rk = zn_rname(res);
        unsigned long rid = zn_declare_resource(s2, rk);
        printf("Declared resource on session 2: %zu %s\n", rid, res);
        rids2[i] = rid;
        free(rk.rname);
    }

    // Declare subscribers and queryabales on second session
    for (unsigned int i = 0; i < RUN; i++)
    {
        zn_reskey_t rk = zn_rid(rids2[i]);
        zn_subscriber_t *sub = zn_declare_subscriber(s2, rk, zn_subinfo_default(), data_handler, NULL);
        assert(sub != NULL);
        printf("Declared subscription on session 2: %zu %s\n", sub->id, res);
        subs2 = _z_list_cons(subs2, sub);
    }

    for (unsigned int i = 0; i < RUN; i++)
    {
        zn_reskey_t rk = zn_rid(rids2[i]);
        zn_queryable_t *qle = zn_declare_queryable(s2, rk, ZN_QUERYABLE_EVAL, query_handler, NULL);
        assert(qle != NULL);
        printf("Declared queryable on session 2: %zu %s\n", qle->id, res);
        qles2 = _z_list_cons(qles2, qle);
    }

    // Declare publisher on firt session
    for (unsigned int i = 0; i < RUN; i++)
    {
        zn_reskey_t rk = zn_rid(rids1[i]);
        zn_publisher_t *pub = zn_declare_publisher(s1, rk);
        assert(pub != NULL);
        printf("Declared publisher on session 1: %zu %s\n", pub->id, res);
        pubs1 = _z_list_cons(pubs1, pub);
    }

    // @TODO: send data

    // Undeclare publishers on first session
    while (pubs1)
    {
        zn_publisher_t *pub = _z_list_head(pubs1);
        printf("Undeclaring publisher on session 2: %zu\n", pub->id);
        zn_undeclare_publisher(pub);
        pubs1 = _z_list_pop(pubs1);
    }

    // Undeclare subscribers and queryables on second session
    while (subs2)
    {
        zn_subscriber_t *sub = _z_list_head(subs2);
        printf("Undeclaring subscription on session 2: %zu\n", sub->id);
        zn_undeclare_subscriber(sub);
        subs2 = _z_list_pop(subs2);
    }

    while (qles2)
    {
        zn_queryable_t *qle = _z_list_head(qles2);
        printf("Undeclaring queryable on session 2: %zu\n", qle->id);
        zn_undeclare_queryable(qle);
        qles2 = _z_list_pop(qles2);
    }

    // Undeclare resources on both sessions
    for (unsigned int i = 0; i < RUN; i++)
    {
        zn_undeclare_resource(s1, rids1[i]);
        printf("Undeclared resource on session 1: %zu\n", rids1[i]);
    }

    for (unsigned int i = 0; i < RUN; i++)
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

    return 0;
}
