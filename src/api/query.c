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

#include "zenoh-pico/session/query.h"

zn_query_consolidation_t zn_query_consolidation_default(void)
{
    zn_query_consolidation_t qc;
    qc.first_routers = zn_consolidation_mode_t_LAZY;
    qc.last_router = zn_consolidation_mode_t_LAZY;
    qc.reception = zn_consolidation_mode_t_FULL;
    return qc;
}

zn_query_consolidation_t zn_query_consolidation_none(void)
{
    zn_query_consolidation_t qc;
    qc.first_routers = zn_consolidation_mode_t_NONE;
    qc.last_router = zn_consolidation_mode_t_NONE;
    qc.reception = zn_consolidation_mode_t_NONE;
    return qc;
}

int zn_query_consolidation_equal(const zn_query_consolidation_t *left, const zn_query_consolidation_t *right)
{
    return memcmp(left, right, sizeof(zn_query_consolidation_t));
}

z_string_t zn_query_predicate(const zn_query_t *query)
{
    z_string_t s;
    s.len = strlen(query->predicate);
    s.val = query->predicate;
    return s;
}

z_string_t zn_query_res_name(const zn_query_t *query)
{
    z_string_t s;
    s.len = strlen(query->rname);
    s.val = query->rname;
    return s;
}

zn_target_t zn_target_default(void)
{
    zn_target_t t;
    t.tag = zn_target_t_BEST_MATCHING;
    return t;
}

zn_query_target_t zn_query_target_default(void)
{
    zn_query_target_t qt;
    qt.kind = ZN_QUERYABLE_ALL_KINDS;
    qt.target = zn_target_default();
    return qt;
}

int zn_query_target_equal(const zn_query_target_t *left, const zn_query_target_t *right)
{
    return memcmp(left, right, sizeof(zn_query_target_t));
}
