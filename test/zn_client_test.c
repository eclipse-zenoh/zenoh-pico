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
#include "zenoh/net.h"
#include "zenoh/net/private/system.h"

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

    zn_session_t *s = zn_open(config);
    assert(s != NULL);

    // Start the read session session lease loops
    znp_start_read_task(s);
    znp_start_lease_task(s);

    for (unsigned int i = 0; i < RUN; i++)
    {
        _z_list_t *subs = _z_list_empty;
        _z_list_t *qles = _z_list_empty;
        unsigned long rids[RUN];

        char res[64];
        for (unsigned int i = 0; i < RUN; i++)
        {
            sprintf(res, "%s%d", uri, i);
            zn_reskey_t rk = zn_rname(res);
            unsigned long rid = zn_declare_resource(s, rk);
            printf("Declared resource: %zu %s\n", rid, res);
            rids[i] = rid;
            free(rk.rname);
        }

        for (unsigned int i = 0; i < RUN; i++)
        {
            zn_reskey_t rk = zn_rid(rids[i]);
            zn_subscriber_t *sub = zn_declare_subscriber(s, rk, zn_subinfo_default(), data_handler, NULL);
            assert(sub != NULL);
            printf("Declared subscription: %zu %s\n", sub->id, res);
            subs = _z_list_cons(subs, sub);
        }

        for (unsigned int i = 0; i < RUN; i++)
        {
            zn_reskey_t rk = zn_rid(rids[i]);
            zn_queryable_t *qle = zn_declare_queryable(s, rk, ZN_QUERYABLE_EVAL, query_handler, NULL);
            assert(qle != NULL);
            printf("Declared queryable: %zu %s\n", qle->id, res);
            qles = _z_list_cons(qles, qle);
        }

        while (subs)
        {
            zn_subscriber_t *sub = _z_list_head(subs);
            printf("Undeclaring subscription: %zu\n", sub->id);
            zn_undeclare_subscriber(sub);
            subs = _z_list_pop(subs);
        }

        while (qles)
        {
            zn_queryable_t *qle = _z_list_head(qles);
            printf("Undeclaring queryable: %zu\n", qle->id);
            zn_undeclare_queryable(qle);
            qles = _z_list_pop(qles);
        }

        for (unsigned int i = 0; i < RUN; i++)
        {
            zn_undeclare_resource(s, rids[i]);
            printf("Undeclared resource: %zu\n", rids[i]);
        }
    }

    znp_stop_lease_task(s);
    znp_stop_read_task(s);

    zn_close(s);

    return 0;
}
