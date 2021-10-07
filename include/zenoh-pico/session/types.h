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

#ifndef _ZENOH_PICO_SESSION_TYPES_H
#define _ZENOH_PICO_SESSION_TYPES_H

#include <stdint.h>
#include <string.h>
#include "zenoh-pico/protocol/types.h"
#include "zenoh-pico/protocol/private/types.h"
#include "zenoh-pico/system/types.h"
#include "zenoh-pico/utils/types.h"
#include "zenoh-pico/link/types.h"

/**
 * The callback signature of the functions handling session discionnection.
 */
typedef void (*zn_on_disconnect_t)(void *zn);

/**
 * A zenoh-net session.
 */
typedef struct
{
    // Socket and internal buffers
    _zn_link_t *link;
    z_mutex_t mutex_rx;
    z_mutex_t mutex_tx;
    z_mutex_t mutex_inner;

    _z_wbuf_t wbuf;
    _z_zbuf_t zbuf;

    _z_wbuf_t dbuf_reliable;
    _z_wbuf_t dbuf_best_effort;

    // Connection state
    z_bytes_t local_pid;
    z_bytes_t remote_pid;

    char *locator;

    volatile z_zint_t lease;
    z_zint_t sn_resolution;
    z_zint_t sn_resolution_half;

    // SN numbers
    z_zint_t sn_tx_reliable;
    z_zint_t sn_tx_best_effort;
    z_zint_t sn_rx_reliable;
    z_zint_t sn_rx_best_effort;

    // Counters
    z_zint_t resource_id;
    z_zint_t entity_id;
    z_zint_t pull_id;
    z_zint_t query_id;

    // Declarations
    z_list_t *local_resources;
    z_list_t *remote_resources;

    z_list_t *local_subscriptions;
    z_list_t *remote_subscriptions;
    z_i_map_t *rem_res_loc_sub_map;

    z_list_t *local_queryables;
    z_i_map_t *rem_res_loc_qle_map;

    z_list_t *pending_queries;

    // Runtime
    zn_on_disconnect_t on_disconnect;

    volatile int read_task_running;
    z_task_t *read_task;

    volatile int lease_task_running;
    volatile int received;
    volatile int transmitted;
    z_task_t *lease_task;
} zn_session_t;

/**
 * Return type when declaring a publisher.
 */
typedef struct
{
    zn_session_t *zn;
    z_zint_t id;
    zn_reskey_t key;
} zn_publisher_t;

/**
 * Return type when declaring a subscriber.
 */
typedef struct
{
    zn_session_t *zn;
    z_zint_t id;
} zn_subscriber_t;

/**
 * Return type when declaring a queryable.
 */
typedef struct
{
    zn_session_t *zn;
    z_zint_t id;
} zn_queryable_t;

/**
 * The query to be answered by a queryable.
 */
typedef struct
{
    zn_session_t *zn;
    z_zint_t qid;
    unsigned int kind;
    const char *rname;
    const char *predicate;
} zn_query_t;

/**
 * The possible values of :c:member:`zn_reply_t.tag`
 *
 *     - **zn_reply_t_Tag_DATA**: The reply contains some data.
 *     - **zn_reply_t_Tag_FINAL**: The reply does not contain any data and indicates that there will be no more replies for this query.
 */
typedef enum zn_reply_t_Tag
{
    zn_reply_t_Tag_DATA,
    zn_reply_t_Tag_FINAL,
} zn_reply_t_Tag;

/**
 * An reply to a :c:func:`zn_query` (or :c:func:`zn_query_collect`).
 *
 * Members:
 *   zn_sample_t data: a :c:type:`zn_sample_t` containing the key and value of the reply.
 *   unsigned int replier_kind: The kind of the replier that sent this reply.
 *   z_bytes_t replier_id: The id of the replier that sent this reply.
 *
 */
typedef struct
{
    zn_sample_t data;
    unsigned int replier_kind;
    z_bytes_t replier_id;
} zn_reply_data_t;

/**
 * An reply to a :c:func:`zn_query`.
 *
 * Members:
 *   zn_reply_t_Tag tag: Indicates if the reply contains data or if it's a FINAL reply.
 *   zn_reply_data_t data: The reply data if :c:member:`zn_reply_t.tag` equals :c:member:`zn_reply_t_Tag.zn_reply_t_Tag_DATA`.
 *
 */
typedef struct
{
    zn_reply_t_Tag tag;
    zn_reply_data_t data;
} zn_reply_t;

/**
 * An array of :c:type:`zn_reply_data_t`.
 * Result of :c:func:`zn_query_collect`.
 *
 * Members:
 *   char *const *val: A pointer to the array.
 *   unsigned int len: The length of the array.
 *
 */
typedef struct
{
    const zn_reply_data_t *val;
    size_t len;
} zn_reply_data_array_t;

/**
 * The callback signature of the functions handling data messages.
 */
typedef void (*zn_data_handler_t)(const zn_sample_t *sample, const void *arg);
/**
 * The callback signature of the functions handling query replies.
 */
typedef void (*zn_query_handler_t)(zn_reply_t reply, const void *arg);
/**
 * The callback signature of the functions handling query messages.
 */
typedef void (*zn_queryable_handler_t)(zn_query_t *query, const void *arg);

#endif /* _ZENOH_PICO_SESSION_TYPES_H */
