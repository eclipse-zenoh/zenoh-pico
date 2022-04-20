//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#ifndef ZENOH_PICO_SESSION_TYPES_H
#define ZENOH_PICO_SESSION_TYPES_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/transport/manager.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/string.h"

#define _ZN_RESOURCE_REMOTE 0
#define _ZN_RESOURCE_IS_LOCAL 1

/**
 * The query to be answered by a queryable.
 */
typedef struct
{
    void *zn; // FIXME: zn_session_t *zn;
    z_zint_t qid;
    unsigned int kind;
    z_str_t rname;
    z_str_t predicate;
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

int _zn_reply_data_eq(const zn_reply_data_t *one, const zn_reply_data_t *two);
void _zn_reply_data_clear(zn_reply_data_t *res);

_Z_ELEM_DEFINE(_zn_reply_data, zn_reply_data_t, _zn_noop_size, _zn_noop_clear, _zn_noop_copy)
_Z_LIST_DEFINE(_zn_reply_data, zn_reply_data_t)

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

typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
} _zn_resource_t;

int _zn_resource_eq(const _zn_resource_t *one, const _zn_resource_t *two);
void _zn_resource_clear(_zn_resource_t *res);

_Z_ELEM_DEFINE(_zn_resource, _zn_resource_t, _zn_noop_size, _zn_resource_clear, _zn_noop_copy)
_Z_LIST_DEFINE(_zn_resource, _zn_resource_t)

/**
 * The callback signature of the functions handling data messages.
 */
typedef void (*zn_data_handler_t)(const zn_sample_t *sample, const void *arg);

typedef struct
{
    z_zint_t id;
    z_str_t rname;
    zn_reskey_t key;
    zn_subinfo_t info;
    zn_data_handler_t callback;
    void *arg;
} _zn_subscriber_t;

int _zn_subscriber_eq(const _zn_subscriber_t *one, const _zn_subscriber_t *two);
void _zn_subscriber_clear(_zn_subscriber_t *sub);

_Z_ELEM_DEFINE(_zn_subscriber, _zn_subscriber_t, _zn_noop_size, _zn_subscriber_clear, _zn_noop_copy)
_Z_LIST_DEFINE(_zn_subscriber, _zn_subscriber_t)

/**
 * The callback signature of the functions handling query messages.
 */
typedef void (*zn_queryable_handler_t)(zn_query_t *query, const void *arg);

typedef struct
{
    z_zint_t id;
    z_str_t rname;
    unsigned int kind;
    zn_queryable_handler_t callback;
    void *arg;
} _zn_queryable_t;

int _zn_queryable_eq(const _zn_queryable_t *one, const _zn_queryable_t *two);
void _zn_queryable_clear(_zn_queryable_t *res);

_Z_ELEM_DEFINE(_zn_queryable, _zn_queryable_t, _zn_noop_size, _zn_queryable_clear, _zn_noop_copy)
_Z_LIST_DEFINE(_zn_queryable, _zn_queryable_t)

typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
} _zn_publisher_t;

typedef struct
{
    zn_reply_t *reply;
    z_timestamp_t tstamp;
} _zn_pending_reply_t;

int _zn_pending_reply_eq(const _zn_pending_reply_t *one, const _zn_pending_reply_t *two);
void _zn_pending_reply_clear(_zn_pending_reply_t *res);

_Z_ELEM_DEFINE(_zn_pending_reply, _zn_pending_reply_t, _zn_noop_size, _zn_pending_reply_clear, _zn_noop_copy)
_Z_LIST_DEFINE(_zn_pending_reply, _zn_pending_reply_t)

/**
 * The callback signature of the functions handling query replies.
 */
typedef void (*zn_query_handler_t)(zn_reply_t reply, const void *arg);

typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
    z_str_t predicate;
    zn_query_target_t target;
    zn_query_consolidation_t consolidation;
    _zn_pending_reply_list_t *pending_replies;
    zn_query_handler_t callback;
    void *arg;
} _zn_pending_query_t;

int _zn_pending_query_eq(const _zn_pending_query_t *one, const _zn_pending_query_t *two);
void _zn_pending_query_clear(_zn_pending_query_t *res);

_Z_ELEM_DEFINE(_zn_pending_query, _zn_pending_query_t, _zn_noop_size, _zn_pending_query_clear, _zn_noop_copy)
_Z_LIST_DEFINE(_zn_pending_query, _zn_pending_query_t)

typedef struct
{
    z_mutex_t mutex;
    z_condvar_t cond_var;
    _zn_reply_data_list_t *replies;
} _zn_pending_query_collect_t;

#endif /* ZENOH_PICO_SESSION_TYPES_H */
