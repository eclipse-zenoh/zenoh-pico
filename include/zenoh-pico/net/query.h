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
    unsigned int _kind;
    _z_keyexpr_t key;
    _z_str_t predicate;
} z_query_t;

/**
 * The callback signature of the functions handling query messages.
 */
typedef void (*_z_questionable_handler_t)(const z_query_t *query, const void *arg);

typedef struct
{
    _z_zint_t _id;
    _z_keyexpr_t _key;
    unsigned int _kind;
    _z_questionable_handler_t _callback;
    void *_arg;
} _z_questionable_t;

int _z_questionable_eq(const _z_questionable_t *one, const _z_questionable_t *two);
void _z_questionable_clear(_z_questionable_t *res);

_Z_ELEM_DEFINE(_z_questionable, _z_questionable_t, _z_noop_size, _z_questionable_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_questionable, _z_questionable_t)

/**
 * Return type when declaring a queryable.
 */
typedef struct
{
    void *_zn;  // FIXME: _z_session_t *zn;
    _z_zint_t _id;
} _z_queryable_t;

/**
 * The kind of consolidation that should be applied on replies to a :c:func:`z_query`
 * at the different stages of the reply process.
 *
 * Members:
 *   first_routers: The consolidation mode to apply on first routers of the replies routing path.
 *   last_router: The consolidation mode to apply on last router of the replies routing path.
 *   reception: The consolidation mode to apply at reception of the replies.
 */
typedef struct
{
    z_consolidation_mode_t first_routers;
    z_consolidation_mode_t last_router;
    z_consolidation_mode_t reception;
} _z_consolidation_strategy_t;

_z_consolidation_strategy_t _z_consolidation_strategy_none(void);
_z_consolidation_strategy_t _z_consolidation_strategy_default(void);

/**
 * Create a default :c:type:`_z_target_t`.
 *
 * Returns:
 *     A :c:type:`_z_target_t` containing the created query target.
 */
_z_target_t _z_target_default(void);

#endif /* ZENOH_PICO_QUERY_NETAPI_H */
