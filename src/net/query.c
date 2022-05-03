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

_z_consolidation_strategy_t _z_consolidation_strategy_default(void)
{
    _z_consolidation_strategy_t qc = { .first_routers = Z_CONSOLIDATION_MODE_LAZY,
                                       .last_router = Z_CONSOLIDATION_MODE_LAZY,
                                       .reception = Z_CONSOLIDATION_MODE_FULL };
    return qc;
}

_z_consolidation_strategy_t _z_consolidation_strategy_none(void)
{
    _z_consolidation_strategy_t qc = { .first_routers = Z_CONSOLIDATION_MODE_NONE,
                                       .last_router = Z_CONSOLIDATION_MODE_NONE,
                                       .reception = Z_CONSOLIDATION_MODE_NONE };
    return qc;
}

_z_string_t _z_query_predicate(const z_query_t *query)
{
    _z_string_t s;
    s.len = strlen(query->_predicate);
    s.val = query->_predicate;
    return s;
}

_z_string_t _z_query_res_name(const z_query_t *query)
{
    _z_string_t s;
    s.len = strlen(query->_rname);
    s.val = query->_rname;
    return s;
}

_z_query_target_t _z_query_target_default(void)
{
    return Z_TARGET_BEST_MATCHING;
}

_z_target_t _z_target_default(void)
{
    return (_z_target_t){._kind = Z_QUERYABLE_ALL_KINDS, .target = _z_query_target_default()};
}
