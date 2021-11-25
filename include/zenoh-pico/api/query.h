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

#ifndef ZENOH_PICO_QUERY_API_H
#define ZENOH_PICO_QUERY_API_H

#include "zenoh-pico/protocol/core.h"

/**
 * Create a default :c:type:`zn_query_consolidation_t`.
 */
zn_query_consolidation_t zn_query_consolidation_default(void);

/**
 * Create a none :c:type:`zn_query_consolidation_t`.
 */
zn_query_consolidation_t zn_query_consolidation_none(void);

/**
 * Check if two :c:type:`zn_query_consolidation_t` are equal
 *
 * Parameters:
 *     right: first :c:type:`zn_query_consolidation_t` to compare
 *     second; second :c:type:`zn_query_consolidation_t` to compare
 *
 * Returns:
 *     0 if equal, or different from 0 if not equal.
 */
int zn_query_consolidation_equal(zn_query_consolidation_t *left, zn_query_consolidation_t *right);

/**
 * Get the predicate of a received query.
 *
 * Parameters:
 *     query: The query.
 *
 * Returns:
 *     The predicate of the query.
 */
z_string_t zn_query_predicate(zn_query_t *query);

/**
 * Get the resource name of a received query.
 *
 * Parameters:
 *     query: The query.
 *
 * Returns:
 *     The resource name of the query.
 */
z_string_t zn_query_res_name(zn_query_t *query);

/**
 * Create a default :c:type:`zn_query_target_t`.
 */
zn_query_target_t zn_query_target_default(void);

/**
 * Check if two :c:type:`zn_query_target_t` are equal
 *
 * Parameters:
 *     right: first :c:type:`zn_query_target_t` to compare
 *     second; second :c:type:`zn_query_target_t` to compare
 *
 * Returns:
 *     0 if equal, or different from 0 if not equal.
 */
int zn_query_target_equal(zn_query_target_t *left, zn_query_target_t *right);

#endif /* ZENOH_PICO_QUERY_API_H */
