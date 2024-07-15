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

#ifndef INCLUDE_ZENOH_PICO_SESSION_SESSION_H
#define INCLUDE_ZENOH_PICO_SESSION_SESSION_H

#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/refcount.h"
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

typedef struct {
    _z_keyexpr_t _key;
    uint16_t _id;
    uint16_t _refcount;
} _z_resource_t;

_Bool _z_resource_eq(const _z_resource_t *one, const _z_resource_t *two);
void _z_resource_clear(_z_resource_t *res);
void _z_resource_copy(_z_resource_t *dst, const _z_resource_t *src);
void _z_resource_free(_z_resource_t **res);

_Z_ELEM_DEFINE(_z_resource, _z_resource_t, _z_noop_size, _z_resource_clear, _z_resource_copy)
_Z_LIST_DEFINE(_z_resource, _z_resource_t)

// Forward declaration to avoid cyclical include
typedef struct _z_sample_t _z_sample_t;

/**
 * The callback signature of the functions handling data messages.
 */
typedef void (*_z_data_handler_t)(const _z_sample_t *sample, void *arg);

typedef struct {
    _z_keyexpr_t _key;
    uint16_t _key_id;
    uint32_t _id;
    _z_data_handler_t _callback;
    _z_drop_handler_t _dropper;
    void *_arg;
    _z_subinfo_t _info;
} _z_subscription_t;

_Bool _z_subscription_eq(const _z_subscription_t *one, const _z_subscription_t *two);
void _z_subscription_clear(_z_subscription_t *sub);

_Z_REFCOUNT_DEFINE(_z_subscription, _z_subscription)
_Z_ELEM_DEFINE(_z_subscriber, _z_subscription_t, _z_noop_size, _z_subscription_clear, _z_noop_copy)
_Z_ELEM_DEFINE(_z_subscription_rc, _z_subscription_rc_t, _z_subscription_rc_size, _z_subscription_rc_drop,
               _z_subscription_rc_copy)
_Z_LIST_DEFINE(_z_subscription_rc, _z_subscription_rc_t)

typedef struct {
    _z_keyexpr_t _key;
    uint32_t _id;
} _z_publication_t;

// Forward type declaration to avoid cyclical include
typedef struct _z_query_rc_t _z_query_rc_t;

/**
 * The callback signature of the functions handling query messages.
 */
typedef void (*_z_queryable_handler_t)(const _z_query_rc_t *query, void *arg);

typedef struct {
    _z_keyexpr_t _key;
    uint32_t _id;
    _z_queryable_handler_t _callback;
    _z_drop_handler_t _dropper;
    void *_arg;
    _Bool _complete;
} _z_session_queryable_t;

_Bool _z_session_queryable_eq(const _z_session_queryable_t *one, const _z_session_queryable_t *two);
void _z_session_queryable_clear(_z_session_queryable_t *res);

_Z_REFCOUNT_DEFINE(_z_session_queryable, _z_session_queryable)
_Z_ELEM_DEFINE(_z_session_queryable, _z_session_queryable_t, _z_noop_size, _z_session_queryable_clear, _z_noop_copy)
_Z_ELEM_DEFINE(_z_session_queryable_rc, _z_session_queryable_rc_t, _z_noop_size, _z_session_queryable_rc_drop,
               _z_noop_copy)
_Z_LIST_DEFINE(_z_session_queryable_rc, _z_session_queryable_rc_t)

// Forward declaration to avoid cyclical includes
typedef struct _z_reply_t _z_reply_t;
typedef _z_list_t _z_reply_data_list_t;
typedef _z_list_t _z_pending_reply_list_t;
typedef struct _z_reply_t _z_reply_t;

/**
 * The callback signature of the functions handling query replies.
 */
typedef void (*_z_reply_handler_t)(const _z_reply_t *reply, void *arg);

typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _id;
    _z_reply_handler_t _callback;
    _z_drop_handler_t _dropper;
    void *_arg;
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
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_t _mutex;
    _z_condvar_t _cond_var;
#endif  // Z_FEATURE_MULTI_THREAD == 1
    _z_reply_data_list_t *_replies;
} _z_pending_query_collect_t;

struct __z_hello_handler_wrapper_t;  // Forward declaration to be used in _z_hello_handler_t
/**
 * The callback signature of the functions handling hello messages.
 */
typedef void (*_z_hello_handler_t)(_z_hello_t *hello, struct __z_hello_handler_wrapper_t *arg);

int8_t _z_session_generate_zid(_z_id_t *bs, uint8_t size);

typedef enum {
    _Z_INTEREST_MSG_TYPE_FINAL = 0,
    _Z_INTEREST_MSG_TYPE_DECL_SUBSCRIBER = 1,
    _Z_INTEREST_MSG_TYPE_DECL_QUERYABLE = 2,
    _Z_INTEREST_MSG_TYPE_DECL_TOKEN = 3,
    _Z_INTEREST_MSG_TYPE_UNDECL_SUBSCRIBER = 4,
    _Z_INTEREST_MSG_TYPE_UNDECL_QUERYABLE = 5,
    _Z_INTEREST_MSG_TYPE_UNDECL_TOKEN = 6,
} _z_interest_msg_type_t;

typedef struct _z_interest_msg_t {
    uint8_t type;
    uint32_t id;
} _z_interest_msg_t;

/**
 * The callback signature of the functions handling interest messages.
 */
typedef void (*_z_interest_handler_t)(const _z_interest_msg_t *msg, void *arg);

typedef struct {
    _z_keyexpr_t _key;
    uint32_t _id;
    _z_interest_handler_t _callback;
    void *_arg;
    uint8_t _flags;
} _z_session_interest_t;

_Bool _z_session_interest_eq(const _z_session_interest_t *one, const _z_session_interest_t *two);
void _z_session_interest_clear(_z_session_interest_t *res);

_Z_REFCOUNT_DEFINE(_z_session_interest, _z_session_interest)
_Z_ELEM_DEFINE(_z_session_interest, _z_session_interest_t, _z_noop_size, _z_session_interest_clear, _z_noop_copy)
_Z_ELEM_DEFINE(_z_session_interest_rc, _z_session_interest_rc_t, _z_noop_size, _z_session_interest_rc_drop,
               _z_noop_copy)
_Z_LIST_DEFINE(_z_session_interest_rc, _z_session_interest_rc_t)

typedef enum {
    _Z_DECLARE_TYPE_SUBSCRIBER = 0,
    _Z_DECLARE_TYPE_QUERYABLE = 1,
    _Z_DECLARE_TYPE_TOKEN = 2,
} _z_declare_type_t;

typedef struct {
    _z_keyexpr_t _key;
    uint32_t _id;
    uint8_t _type;
} _z_declare_data_t;

void _z_declare_data_clear(_z_declare_data_t *data);
_Z_ELEM_DEFINE(_z_declare_data, _z_declare_data_t, _z_noop_size, _z_declare_data_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_declare_data, _z_declare_data_t)

#endif /* INCLUDE_ZENOH_PICO_SESSION_SESSION_H */
