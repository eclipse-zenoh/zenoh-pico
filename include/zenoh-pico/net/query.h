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

/**
 * Return type when declaring a queryable.
 */
typedef struct
{
    void *zn;  // FIXME: zn_session_t *zn;
    z_zint_t id;
} zn_queryable_t;

/**
 * Create a default :c:type:`zn_query_consolidation_t`.
 *
 * Returns:
 *     A :c:type:`zn_query_consolidation_t` containing the created query
 *     consolidation.
 */
zn_query_consolidation_t zn_query_consolidation_default(void);

/**
 * Create a none :c:type:`zn_query_consolidation_t`.
 *
 * Returns:
 *     A :c:type:`zn_query_consolidation_t` containing the created query
 *     consolidation.
 */
zn_query_consolidation_t zn_query_consolidation_none(void);

/**
 * Check if two :c:type:`zn_query_consolidation_t` are equal
 *
 * Parameters:
 *     left: The first :c:type:`zn_query_consolidation_t` to compare.
 *           The caller keeps its ownership.
 *     right: The second :c:type:`zn_query_consolidation_t` to compare.
 *            The caller keeps its ownership.
 *
 * Returns:
 *     0 if equal, or different from 0 if not equal.
 */
int zn_query_consolidation_equal(const zn_query_consolidation_t *left, const zn_query_consolidation_t *right);

/**
 * Get the predicate of a received query.
 *
 * Parameters:
 *     query: The query. The caller keeps its ownership.
 *
 * Returns:
 *     A :c:type:`z_string_t` containing the predicate of the query.
 *     Note that, the predicate is provided as a reference, thus should not be
 *     modified.
 */
z_string_t zn_query_predicate(const zn_query_t *query);

/**
 * Get the resource name of a received query.
 *
 * Parameters:
 *     query: The query. The caller keeps its ownership.
 *
 * Returns:
 *     A :c:type:`z_string_t` containing the resource name of the query.
 *     Note that, the resource name is provided as a reference, thus should
 *     not be modified.
 */
z_string_t zn_query_res_name(const zn_query_t *query);

/**
 * Create a default :c:type:`zn_query_target_t`.
 *
 * Returns:
 *     A :c:type:`zn_query_target_t` containing the created query
 *     target.
 */
zn_query_target_t zn_query_target_default(void);

/**
 * Check if two :c:type:`zn_query_target_t` are equal
 *
 * Parameters:
 *     left: The first :c:type:`zn_query_target_t` to compare.
 *           The caller keeps its ownership.
 *     right: The second :c:type:`zn_query_target_t` to compare.
 *            The caller keeps its ownership.
 *
 * Returns:
 *     0 if equal, or different from 0 if not equal.
 */
int zn_query_target_equal(const zn_query_target_t *left, const zn_query_target_t *right);

#endif /* ZENOH_PICO_QUERY_NETAPI_H */
