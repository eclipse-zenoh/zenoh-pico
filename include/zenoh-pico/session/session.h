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

#include <stdbool.h>

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/pointer.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/transport/manager.h"

/**
 * The callback signature of the cleanup functions.
 */
typedef void (*_z_drop_handler_t)(void *arg);

#define _Z_RESOURCE_IS_REMOTE 0
#define _Z_RESOURCE_IS_LOCAL 1

/**
 * An reply to a :c:func:`z_query`.
 *
 * Members:
 *   _z_sample_t data: a :c:type:`_z_sample_t` containing the key and value of the reply.
 *   _z_bytes_t replier_id: The id of the replier that sent this reply.
 *
 */
typedef struct {
    _z_sample_t sample;
    _z_bytes_t replier_id;
} _z_reply_data_t;

void _z_reply_data_clear(_z_reply_data_t *rd);

_Z_ELEM_DEFINE(_z_reply_data, _z_reply_data_t, _z_noop_size, _z_reply_data_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_reply_data, _z_reply_data_t)
_Z_ARRAY_DEFINE(_z_reply_data, _z_reply_data_t)

/**
 * An reply to a :c:func:`z_query`.
 *
 * Members:
 *   _z_reply_t_Tag tag: Indicates if the reply contains data or if it's a FINAL reply.
 *   _z_reply_data_t data: The reply data if :c:member:`_z_reply_t.tag` equals
 * :c:member:`_z_reply_t_Tag.Z_REPLY_TAG_DATA`.
 *
 */
typedef struct {
    _z_reply_data_t data;
    z_reply_tag_t _tag;
} _z_reply_t;
void _z_reply_clear(_z_reply_t *src);
void _z_reply_free(_z_reply_t **hello);

typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _id;
} _z_resource_t;

_Bool _z_resource_eq(const _z_resource_t *one, const _z_resource_t *two);
void _z_resource_clear(_z_resource_t *res);
void _z_resource_free(_z_resource_t **res);

_Z_ELEM_DEFINE(_z_resource, _z_resource_t, _z_noop_size, _z_resource_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_resource, _z_resource_t)

/**
 * The callback signature of the functions handling data messages.
 */
typedef void (*_z_data_handler_t)(const _z_sample_t *sample, void *arg);

typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _id;
    _z_data_handler_t _callback;
    _z_drop_handler_t _dropper;
    void *_arg;
    _z_subinfo_t _info;
} _z_subscription_t;

_Bool _z_subscription_eq(const _z_subscription_t *one, const _z_subscription_t *two);
void _z_subscription_clear(_z_subscription_t *sub);

_Z_POINTER_DEFINE(_z_subscription, _z_subscription);
_Z_ELEM_DEFINE(_z_subscriber, _z_subscription_t, _z_noop_size, _z_subscription_clear, _z_noop_copy)
_Z_ELEM_DEFINE(_z_subscription_sptr, _z_subscription_sptr_t, _z_noop_size, _z_subscription_sptr_drop, _z_noop_copy)
_Z_LIST_DEFINE(_z_subscription_sptr, _z_subscription_sptr_t)

typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _id;
} _z_publication_t;

/**
 * The callback signature of the functions handling query messages.
 */
typedef void (*_z_questionable_handler_t)(const z_query_t *query, void *arg);

typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _id;
    _z_questionable_handler_t _callback;
    _z_drop_handler_t _dropper;
    void *_arg;
    _Bool _complete;
} _z_questionable_t;

_Bool _z_questionable_eq(const _z_questionable_t *one, const _z_questionable_t *two);
void _z_questionable_clear(_z_questionable_t *res);

_Z_POINTER_DEFINE(_z_questionable, _z_questionable);
_Z_ELEM_DEFINE(_z_questionable, _z_questionable_t, _z_noop_size, _z_questionable_clear, _z_noop_copy)
_Z_ELEM_DEFINE(_z_questionable_sptr, _z_questionable_sptr_t, _z_noop_size, _z_questionable_sptr_drop, _z_noop_copy)
_Z_LIST_DEFINE(_z_questionable_sptr, _z_questionable_sptr_t)

typedef struct {
    _z_reply_t _reply;
    _z_timestamp_t _tstamp;
} _z_pending_reply_t;

_Bool _z_pending_reply_eq(const _z_pending_reply_t *one, const _z_pending_reply_t *two);
void _z_pending_reply_clear(_z_pending_reply_t *res);

_Z_ELEM_DEFINE(_z_pending_reply, _z_pending_reply_t, _z_noop_size, _z_pending_reply_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_pending_reply, _z_pending_reply_t)

struct __z_reply_handler_wrapper_t;  // Forward declaration to be used in _z_reply_handler_t
/**
 * The callback signature of the functions handling query replies.
 */
typedef void (*_z_reply_handler_t)(_z_reply_t *reply, struct __z_reply_handler_wrapper_t *arg);

typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _id;
    _z_reply_handler_t _callback;
    _z_drop_handler_t _dropper;
    void *_call_arg;  // TODO[API-NET]: These two can be merged into one, when API and NET are a single layer
    void *_drop_arg;  // TODO[API-NET]: These two can be merged into one, when API and NET are a single layer
    char *_parameters;
    _z_pending_reply_list_t *_pending_replies;
    z_query_target_t _target;
    z_consolidation_mode_t _consolidation;
    _Bool _anykey;
} _z_pending_query_t;

_Bool _z_pending_query_eq(const _z_pending_query_t *one, const _z_pending_query_t *two);
void _z_pending_query_clear(_z_pending_query_t *res);

_Z_ELEM_DEFINE(_z_pending_query, _z_pending_query_t, _z_noop_size, _z_pending_query_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_pending_query, _z_pending_query_t)

typedef struct {
#if Z_MULTI_THREAD == 1
    _z_mutex_t _mutex;
    _z_condvar_t _cond_var;
#endif  // Z_MULTI_THREAD == 1
    _z_reply_data_list_t *_replies;
} _z_pending_query_collect_t;

struct __z_hello_handler_wrapper_t;  // Forward declaration to be used in _z_hello_handler_t
/**
 * The callback signature of the functions handling hello messages.
 */
typedef void (*_z_hello_handler_t)(_z_hello_t *hello, struct __z_hello_handler_wrapper_t *arg);

int8_t _z_session_generate_zid(_z_bytes_t *bs, uint8_t size);

#endif /* ZENOH_PICO_SESSION_TYPES_H */
