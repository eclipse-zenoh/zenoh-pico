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

#ifndef ZENOH_PICO_API_TYPES_H
#define ZENOH_PICO_API_TYPES_H

#include "zenoh-pico/net/publish.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"
#include "zenoh-pico/protocol/core.h"

/**
 * Represents a variable-length encoding unsigned integer.
 *
 * It is equivalent to the size of a ``size_t``.
 */
typedef _z_zint_t z_zint_t;

/**
 * Represents an array of bytes.
 *
 * Members:
 *   size_t len: The length of the bytes array.
 *   uint8_t *start: A pointer to the bytes array.
 */
typedef _z_bytes_t z_bytes_t;

/**
 * Represents a Zenoh ID.
 *
 * In general, valid Zenoh IDs are LSB-first 128bit unsigned and non-zero integers.
 *
 * Members:
 *   uint8_t id[16]: The array containing the 16 octets of a Zenoh ID.
 */
typedef struct {
    uint8_t id[16];
} z_id_t;

/**
 * Represents a string without null-terminator.
 *
 * Members:
 *   size_t len: The length of the string.
 *   const char *val: A pointer to the string.
 */
typedef _z_string_t z_string_t;

/**
 * Represents a key expression in Zenoh.
 *
 * Operations over :c:type:`z_keyexpr_t` must be done using the provided functions:
 *
 *   - :c:func:`z_keyexpr`
 *   - :c:func:`z_keyexpr_is_valid`
 *   - :c:func:`z_keyexpr_to_string`
 *   - :c:func:`zp_keyexpr_resolve`
 */
typedef _z_keyexpr_t z_keyexpr_t;

/**
 * Represents a Zenoh configuration.
 *
 * Configurations are usually used to set the parameters of a Zenoh session upon its opening.
 *
 * Operations over :c:type:`z_config_t` must be done using the provided functions:
 *
 *   - :c:func:`z_config_new`
 *   - :c:func:`z_config_empty`
 *   - :c:func:`z_config_default`
 *   - :c:func:`zp_keyexpr_resolve`
 *   - :c:func:`zp_config_get`
 *   - :c:func:`zp_config_insert`
 */
typedef _z_config_t z_config_t;

/**
 * Represents a Zenoh session.
 */
typedef _z_session_t z_session_t;

/**
 * Represents a Zenoh (push) Subscriber entity.
 *
 * Operations over :c:type:`z_subscriber_t` must be done using the provided functions:
 *
 *   - :c:func:`z_declare_subscriber`
 *   - :c:func:`z_undeclare_subscriber`
 */
typedef _z_subscriber_t z_subscriber_t;

/**
 * Represents a Zenoh Pull Subscriber entity.
 *
 * Operations over :c:type:`z_pull_subscriber_t` must be done using the provided functions:
 *
 *   - :c:func:`z_declare_pull_subscriber`
 *   - :c:func:`z_undeclare_pull_subscriber`
 *   - :c:func:`z_subscriber_pull`
 */
typedef _z_subscriber_t z_pull_subscriber_t;

/**
 * Represents a Zenoh Publisher entity.
 *
 * Operations over :c:type:`z_publisher_t` must be done using the provided functions:
 *
 *   - :c:func:`z_declare_publisher`
 *   - :c:func:`z_undeclare_publisher`
 *   - :c:func:`z_publisher_put`
 *   - :c:func:`z_publisher_delete`
 */
typedef _z_publisher_t z_publisher_t;

/**
 * Represents a Zenoh Queryable entity.
 *
 * Operations over :c:type:`z_queryable_t` must be done using the provided functions:
 *
 *   - :c:func:`z_declare_queryable`
 *   - :c:func:`z_undeclare_queryable`
 */
typedef _z_queryable_t z_queryable_t;

/**
 * Represents the encoding of a payload, in a MIME-like format.
 *
 * Members:
 *   z_encoding_prefix_t prefix: The integer prefix of this encoding.
 *   z_bytes_t suffix: The suffix of this encoding. It MUST be a valid UTF-8 string.
 */
typedef _z_encoding_t z_encoding_t;

/**
 * Represents a Zenoh value.
 *
 * Members:
 *   z_bytes_t payload: The payload of this zenoh value.
 *   z_encoding_t encoding: The encoding of the `payload`.
 */
typedef struct {
    z_bytes_t payload;
    z_encoding_t encoding;
} z_value_t;

/**
 * Represents the set of options that can be applied to a (push) subscriber,
 * upon its declaration via :c:func:`z_declare_subscriber`.
 *
 * Members:
 *   z_reliability_t reliability: The subscription reliability.
 */
typedef struct {
    z_reliability_t reliability;
} z_subscriber_options_t;

/**
 * Represents the set of options that can be applied to a pull subscriber,
 * upon its declaration via :c:func:`z_declare_pull_subscriber`.
 *
 * Members:
 *   z_reliability_t reliability: The subscription reliability.
 */
typedef struct {
    z_reliability_t reliability;
} z_pull_subscriber_options_t;

/**
 * Represents the replies consolidation to apply on replies to a :c:func:`z_get`.
 *
 * Members:
 *   z_query_consolidation_tag_t tag: Defines if the replies consolidation strategy is automatically
 *                                    or manually applied.
 *   z_consolidation_mode_t mode: Defines the consolidation mode to apply to the replies.
 */
typedef struct {
    z_consolidation_mode_t mode;
} z_query_consolidation_t;

/**
 * Represents the set of options that can be applied to a publisher,
 * upon its declaration via :c:func:`z_declare_publisher`.
 *
 * Members:
 *   int8_t local_routing: ``0``: disabled, ``1``: enabled, ``-1``: apply session level config.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing messages from this
 * publisher. z_priority_t priority: The priority of messages issued by this publisher.
 */
typedef struct {
    int8_t local_routing;
    z_congestion_control_t congestion_control;
    z_priority_t priority;
} z_publisher_options_t;

/**
 * Represents the set of options that can be applied to a queryable,
 * upon its declaration via :c:func:`z_declare_queryable`.
 *
 * Members:
 *   bool complete: The completeness of the queryable.
 */
typedef struct {
    bool complete;
} z_queryable_options_t;

/**
 * Represents the set of options that can be applied to a query reply,
 * sent via :c:func:`z_query_reply`.
 *
 * Members:
 *   z_encoding_t encoding: The encoding of the payload.
 */
typedef struct {
    z_encoding_t encoding;
} z_query_reply_options_t;

/**
 * Represents the set of options that can be applied to the put operation,
 * whenever issued via :c:func:`z_put`.
 *
 * Members:
 *   z_encoding_t encoding: The encoding of the payload.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 */
typedef struct {
    z_encoding_t encoding;
    z_congestion_control_t congestion_control;
    z_priority_t priority;
} z_put_options_t;

/**
 * Represents the set of options that can be applied to the delete operation,
 * whenever issued via :c:func:`z_delete`.
 *
 * Members:
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when router.
 */
typedef struct {
    z_congestion_control_t congestion_control;
    z_priority_t priority;
} z_delete_options_t;

/**
 * Represents the set of options that can be applied to the put operation by a previously declared publisher,
 * whenever issued via :c:func:`z_publisher_put`.
 *
 * Members:
 *   z_encoding_t encoding: The encoding of the payload.
 */
typedef struct {
    z_encoding_t encoding;
} z_publisher_put_options_t;

/**
 * Represents the set of options that can be applied to the delete operation by a previously declared publisher,
 * whenever issued via :c:func:`z_publisher_delete`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} z_publisher_delete_options_t;

/**
 * Represents the set of options that can be applied to the get operation,
 * whenever issued via :c:func:`z_get`.
 *
 * Members:
 *   z_query_target_t target: The queryables that should be targeted by this get.
 *   z_query_consolidation_t consolidation: The replies consolidation strategy to apply on replies.
 */
typedef struct {
    z_query_target_t target;
    z_query_consolidation_t consolidation;
} z_get_options_t;

/**
 * Represents the set of options that can be applied to the read task,
 * whenever issued via :c:func:`zp_start_read_task`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_task_read_options_t;

/**
 * Represents the set of options that can be applied to the lease task,
 * whenever issued via :c:func:`zp_start_lease_task`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_task_lease_options_t;

/**
 * Represents the set of options that can be applied to the read operation,
 * whenever issued via :c:func:`zp_read`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_read_options_t;

/**
 * Represents the set of options that can be applied to the keep alive send,
 * whenever issued via :c:func:`zp_send_keep_alive`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_send_keep_alive_options_t;

/**
 * Represents the set of options that can be applied to the join send,
 * whenever issued via :c:func:`zp_send_join`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_send_join_options_t;

/**
 * Represents a data sample.
 *
 * A sample is the value associated to a given :c:type:`z_keyexpr_t` at a given point in time.
 *
 * Members:
 *   z_keyexpr_t keyexpr: The keyexpr of this data sample.
 *   z_bytes_t payload: The value of this data sample.
 *   z_encoding_t encoding: The encoding of the value of this data sample.
 *   z_sample_kind_t kind: The kind of this data sample (PUT or DELETE).
 *   z_timestamp_t timestamp: The timestamp of this data sample.
 */
typedef _z_sample_t z_sample_t;

/**
 * Represents the content of a `hello` message returned by a zenoh entity as a reply to a `scout` message.
 *
 * Members:
 *   unsigned int whatami: The kind of zenoh entity.
 *   z_bytes_t pid: The Zenoh ID of the scouted entity (empty if absent).
 *   z_str_array_t locators: The locators of the scouted entity.
 */
typedef _z_hello_t z_hello_t;

/**
 * Represents the content of a reply to a query.
 *
 * Members:
 *   z_sample_t sample: The :c:type:`_z_sample_t` containing the key and value of the reply.
 *   z_bytes_t replier_id: The id of the replier that sent this reply.
 */
typedef _z_reply_data_t z_reply_data_t;

/**
 * Represents the reply to a query.
 *
 * Members:
 *   z_reply_data_t data: the content of the reply.
 */
typedef _z_reply_t z_reply_t;

#define _TYPEDEF_ARRAY(type, alias, elem, name)                                                                \
    typedef type alias;                                                                                        \
    static inline elem *z_##name##_array_get(const alias *a, size_t k) { return _z_##name##_array_get(a, k); } \
    static inline size_t z_##name##_array_len(const alias *a) { return _z_##name##_array_len(a); }             \
    static inline uint8_t z_##name##_array_is_empty(const alias *a) { return _z_##name##_array_is_empty(a); }

/**
 * Represents an array of ``char *``.
 *
 * Operations over :c:type:`z_str_array_t` must be done using the provided functions:
 *
 *   - ``char *z_str_array_get(z_str_array_t *a, size_t k);``
 *   - ``size_t z_str_array_len(z_str_array_t *a);``
 *   - ``uint8_t z_str_array_array_is_empty(z_str_array_t *a);``
 */
_TYPEDEF_ARRAY(_z_str_array_t, z_str_array_t, char *, str)

/**
 * Represents an array of ``z_hello_t``.
 *
 * Operations over :c:type:`z_hello_array_t` must be done using the provided functions:
 *
 *   - ``z_hello_t *z_hello_array_get(z_hello_array_t *a, size_t k);``
 *   - ``size_t z_hello_array_len(z_hello_array_t *a);``
 *   - ``uint8_t z_hello_array_array_is_empty(z_hello_array_t *a);``
 */
_TYPEDEF_ARRAY(_z_hello_array_t, z_hello_array_t, z_hello_t, hello)

/**
 * Represents an array of ``z_reply_data_t``.
 *
 * Operations over :c:type:`z_reply_data_array_t` must be done using the provided functions:
 *
 *   - ``z_reply_data_t *z_reply_data_array_get(z_reply_data_array_t *a, size_t k);``
 *   - ``size_t z_reply_data_array_len(z_reply_data_array_t *a);``
 *   - ``uint8_t z_reply_data_array_array_is_empty(z_reply_data_array_t *a);``
 */
_TYPEDEF_ARRAY(_z_reply_data_array_t, z_reply_data_array_t, z_reply_data_t, reply_data)

/* Owned types */
#define _OWNED_TYPE(type, name) \
    typedef struct {            \
        type *_value;           \
    } z_owned_##name##_t;

_OWNED_TYPE(z_bytes_t, bytes)
_OWNED_TYPE(z_string_t, string)
_OWNED_TYPE(z_keyexpr_t, keyexpr)

_OWNED_TYPE(z_config_t, config)
_OWNED_TYPE(z_session_t, session)
_OWNED_TYPE(z_subscriber_t, subscriber)
_OWNED_TYPE(z_pull_subscriber_t, pull_subscriber)
_OWNED_TYPE(z_publisher_t, publisher)
_OWNED_TYPE(z_queryable_t, queryable)

_OWNED_TYPE(z_encoding_t, encoding)
_OWNED_TYPE(z_query_target_t, query_target)
_OWNED_TYPE(z_query_consolidation_t, query_consolidation)
_OWNED_TYPE(z_put_options_t, put_options)

_OWNED_TYPE(z_sample_t, sample)
_OWNED_TYPE(z_hello_t, hello)
_OWNED_TYPE(z_reply_t, reply)
_OWNED_TYPE(z_reply_data_t, reply_data)

_OWNED_TYPE(z_str_array_t, str_array)
_OWNED_TYPE(z_hello_array_t, hello_array)
_OWNED_TYPE(z_reply_data_array_t, reply_data_array)

typedef void (*_z_dropper_handler_t)(void *arg);

/**
 * Represents the sample closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   void *context: a pointer to an arbitrary state.
 *   _z_data_handler_t call: `void *call(const struct z_sample_t*, const void *context)` is the callback function.
 *   _z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 */
typedef struct {
    void *context;
    _z_data_handler_t call;
    _z_dropper_handler_t drop;
} z_owned_closure_sample_t;

/**
 * Represents the query callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   void *context: a pointer to an arbitrary state.
 *   _z_questionable_handler_t call: `void (*_z_questionable_handler_t)(z_query_t *query, void *arg)` is the callback
 * function. _z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 */
typedef struct {
    void *context;
    _z_questionable_handler_t call;
    _z_dropper_handler_t drop;
} z_owned_closure_query_t;

typedef void (*z_owned_reply_handler_t)(z_owned_reply_t reply, void *arg);

/**
 * Represents the query reply callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   void *context: a pointer to an arbitrary state.
 *   z_owned_reply_handler_t call: `void (*z_owned_reply_handler_t)(z_owned_reply_t reply, void *arg)` is the callback
 * function. _z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 */
typedef struct {
    void *context;
    z_owned_reply_handler_t call;
    _z_dropper_handler_t drop;
} z_owned_closure_reply_t;

typedef void (*z_id_handler_t)(const z_id_t *id, void *arg);

/**
 * Represents the Zenoh ID callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   void *context: a pointer to an arbitrary state.
 *   z_id_handler_t call: `void (*z_id_handler_t)(const z_id_t *id, void *arg)` is the callback function.
 *   _z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 */
typedef struct {
    void *context;
    z_id_handler_t call;
    _z_dropper_handler_t drop;
} z_owned_closure_zid_t;

#endif /* ZENOH_PICO_API_TYPES_H */
