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

#ifndef ZENOH_PICO_QUERY_NETAPI_H
#define ZENOH_PICO_QUERY_NETAPI_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/api/constants.h"

/**
 * The query to be answered by a queryable.
 */
typedef struct
{
    void *_zn;  // FIXME: _z_session_t *zn;
    _z_zint_t _qid;
    z_queryable_kind_t kind;
    _z_keyexpr_t key;
    char *predicate;
} z_query_t;

/**
 * Return type when declaring a queryable.
 */
typedef struct
{
    void *_zn;  // FIXME: _z_session_t *zn;
    _z_zint_t _id;
} _z_queryable_t;

typedef struct
{
    z_queryable_kind_t _kind;
    z_query_target_t _target;
} _z_target_t;

_z_target_t _z_query_target_default(void);
z_consolidation_strategy_t _z_consolidation_strategy_none(void);
z_consolidation_strategy_t _z_consolidation_strategy_default(void);

#endif /* ZENOH_PICO_QUERY_NETAPI_H */
