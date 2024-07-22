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
//   Błażej Sowa, <blazej@fictionlab.pl>
//
#ifndef INCLUDE_ZENOH_PICO_API_TYPES_H
#define INCLUDE_ZENOH_PICO_API_TYPES_H

#include "olv_macros.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/net/publish.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/net/reply.h"
#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"
#include "zenoh-pico/protocol/core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a variable-length encoding unsigned integer.
 *
 * It is equivalent to the size of a ``size_t``.
 */
typedef _z_zint_t z_zint_t;

/**
 * Represents a Zenoh ID.
 *
 * In general, valid Zenoh IDs are LSB-first 128bit unsigned and non-zero integers.
 *
 * Members:
 *   uint8_t id[16]: The array containing the 16 octets of a Zenoh ID.
 */
typedef _z_id_t z_id_t;

/*
 * Represents timestamp value in Zenoh
 */
typedef _z_timestamp_t z_timestamp_t;

/**
 * Represents an array of bytes.
 *
 * Members:
 *   size_t len: The length of the bytes array.
 *   uint8_t *start: A pointer to the bytes array.
 */
_Z_OWNED_TYPE_VALUE(_z_slice_t, slice)
_Z_LOANED_TYPE(_z_slice_t, slice)

/**
 * Represents a container for slices.
 */
_Z_OWNED_TYPE_VALUE(_z_bytes_t, bytes)
_Z_LOANED_TYPE(_z_bytes_t, bytes)

/**
 * Represents a writer for serialized data.
 */
_Z_OWNED_TYPE_VALUE(_z_bytes_writer_t, bytes_writer)
_Z_LOANED_TYPE(_z_bytes_writer_t, bytes_writer)

/**
 * An iterator over multi-element serialized data.
 */
typedef _z_bytes_iterator_t z_bytes_iterator_t;

/**
 * A reader for serialized data.
 */
typedef _z_bytes_reader_t z_bytes_reader_t;

/**
 * Represents a string without null-terminator.
 *
 * Members:
 *   size_t len: The length of the string.
 *   const char *val: A pointer to the string.
 */
_Z_OWNED_TYPE_VALUE(_z_string_t, string)
_Z_LOANED_TYPE(_z_string_t, string)
_Z_VIEW_TYPE(_z_string_t, string)

/**
 * Represents a key expression in Zenoh.
 *
 * Members are private and operations must be done using the provided functions:
 *
 *   - :c:func:`z_keyexpr_from_str`
 *   - :c:func:`z_keyexpr_as_view_string`
 */
_Z_OWNED_TYPE_VALUE(_z_keyexpr_t, keyexpr)
_Z_LOANED_TYPE(_z_keyexpr_t, keyexpr)
_Z_VIEW_TYPE(_z_keyexpr_t, keyexpr)

/**
 * Represents a Zenoh configuration, used to configure Zenoh sessions upon opening.
 *
 * Members are private and operations must be done using the provided functions:
 *
 *   - :c:func:`z_config_new`
 *   - :c:func:`z_config_default`
 *   - :c:func:`zp_config_get`
 *   - :c:func:`zp_config_insert`
 */
_Z_OWNED_TYPE_VALUE(_z_config_t, config)
_Z_LOANED_TYPE(_z_config_t, config)

/**
 * Represents a Zenoh Session.
 */
_Z_OWNED_TYPE_RC(_z_session_rc_t, session)
_Z_LOANED_TYPE(_z_session_rc_t, session)

/**
 * Represents a Zenoh Subscriber entity.
 *
 * Members are private and operations must be done using the provided functions:
 *
 *   - :c:func:`z_declare_subscriber`
 *   - :c:func:`z_undeclare_subscriber`
 */
_Z_OWNED_TYPE_VALUE(_z_subscriber_t, subscriber)
_Z_LOANED_TYPE(_z_subscriber_t, subscriber)

/**
 * Represents a Zenoh Publisher entity.
 *
 * Members are private and operations must be done using the provided functions:
 *
 *   - :c:func:`z_declare_publisher`
 *   - :c:func:`z_undeclare_publisher`
 *   - :c:func:`z_publisher_put`
 *   - :c:func:`z_publisher_delete`
 */
_Z_OWNED_TYPE_VALUE(_z_publisher_t, publisher)
_Z_LOANED_TYPE(_z_publisher_t, publisher)

/**
 * Represents a Zenoh Queryable entity.
 *
 * Members are private and operations must be done using the provided functions:
 *
 *   - :c:func:`z_declare_queryable`
 *   - :c:func:`z_undeclare_queryable`
 */
_Z_OWNED_TYPE_VALUE(_z_queryable_t, queryable)
_Z_LOANED_TYPE(_z_queryable_t, queryable)

/**
 * Represents a Zenoh Query entity, received by Zenoh Queryable entities.
 *
 */
_Z_OWNED_TYPE_RC(_z_query_rc_t, query)
_Z_LOANED_TYPE(_z_query_rc_t, query)

/**
 * Represents the encoding of a payload, in a MIME-like format.
 *
 * Members:
 *   uint16_t prefix: The integer prefix of this encoding.
 *   z_loaned_slice_t* suffix: The suffix of this encoding. It MUST be a valid UTF-8 string.
 */
_Z_OWNED_TYPE_VALUE(_z_encoding_t, encoding)
_Z_LOANED_TYPE(_z_encoding_t, encoding)

/**
 * Represents a Zenoh reply error.
 *
 * Members:
 *   z_loaned_encoding_t encoding: The encoding of the error `payload`.
 *   z_loaned_bytes_t* payload: The payload of this zenoh reply error.
 */
_Z_OWNED_TYPE_VALUE(_z_value_t, reply_err)
_Z_LOANED_TYPE(_z_value_t, reply_err)

/**
 * Represents the configuration used to configure a subscriber upon declaration :c:func:`z_declare_subscriber`.
 *
 * Members:
 *   z_reliability_t reliability: The subscription reliability value.
 */
typedef struct {
    z_reliability_t reliability;
} z_subscriber_options_t;

/**
 * Represents the reply consolidation mode to apply on replies to a :c:func:`z_get`.
 *
 * Members:
 *   z_consolidation_mode_t mode: the consolidation mode, see :c:type:`z_consolidation_mode_t`
 */
typedef struct {
    z_consolidation_mode_t mode;
} z_query_consolidation_t;

/**
 * Represents the configuration used to configure a publisher upon declaration with :c:func:`z_declare_publisher`.
 *
 * Members:
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing messages from this
 * publisher.
 *   z_priority_t priority: The priority of messages issued by this publisher.
 *   _Bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 */
typedef struct {
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    _Bool is_express;
} z_publisher_options_t;

/**
 * Represents the configuration used to configure a queryable upon declaration :c:func:`z_declare_queryable`.
 *
 * Members:
 *   _Bool complete: The completeness of the queryable.
 */
typedef struct {
    _Bool complete;
} z_queryable_options_t;

/**
 * Represents the configuration used to configure a query reply sent via :c:func:`z_query_reply.
 *
 * Members:
 *   z_owned_encoding_t *encoding: The encoding of the payload.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   _Bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_owned_bytes_t *attachment: An optional attachment to the response.
 */
typedef struct {
    z_owned_encoding_t *encoding;
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    z_timestamp_t *timestamp;
    _Bool is_express;
    z_owned_bytes_t *attachment;
} z_query_reply_options_t;

/**
 * Represents the configuration used to configure a query reply delete sent via :c:func:`z_query_reply_del.
 *
 * Members:
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   _Bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_owned_bytes_t *attachment: An optional attachment to the response.
 */
typedef struct {
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    z_timestamp_t *timestamp;
    _Bool is_express;
    z_owned_bytes_t *attachment;
} z_query_reply_del_options_t;

/**
 * Represents the configuration used to configure a query reply error sent via :c:func:`z_query_reply_err.
 *
 * Members:
 *   z_owned_encoding_t *encoding: The encoding of the payload.
 */
typedef struct {
    z_owned_encoding_t *encoding;
} z_query_reply_err_options_t;

/**
 * Represents the configuration used to configure a put operation sent via via :c:func:`z_put`.
 *
 * Members:
 *   z_owned_encoding_t *encoding: The encoding of the payload.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   _Bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_owned_bytes_t *attachment: An optional attachment to the publication.
 */
typedef struct {
    z_owned_encoding_t *encoding;
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    z_timestamp_t *timestamp;
    _Bool is_express;
    z_owned_bytes_t *attachment;
} z_put_options_t;

/**
 * Represents the configuration used to configure a delete operation sent via :c:func:`z_delete`.
 *
 * Members:
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when router.
 *   _Bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 */
typedef struct {
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    _Bool is_express;
    z_timestamp_t *timestamp;
} z_delete_options_t;

/**
 * Represents the configuration used to configure a put operation by a previously declared publisher,
 * sent via :c:func:`z_publisher_put`.
 *
 * Members:
 *   z_owned_encoding_t *encoding: The encoding of the payload.
 *   _Bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   z_owned_bytes_t *attachment: An optional attachment to the publication.
 */
typedef struct {
    z_owned_encoding_t *encoding;
    _Bool is_express;
    z_timestamp_t *timestamp;
    z_owned_bytes_t *attachment;
} z_publisher_put_options_t;

/**
 * Represents the configuration used to configure a delete operation by a previously declared publisher,
 * sent via :c:func:`z_publisher_delete`.
 *
 * Members:
 *   _Bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 */
typedef struct {
    _Bool is_express;
    z_timestamp_t *timestamp;
} z_publisher_delete_options_t;

/**
 * Represents the configuration used to configure a get operation sent via :c:func:`z_get`.
 *
 * Members:
 *   z_owned_bytes_t payload: The payload to include in the query.
 *   z_owned_encoding_t *encoding: Payload encoding.
 *   z_query_consolidation_t consolidation: The replies consolidation strategy to apply on replies.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing the query.
 *   z_priority_t priority: The priority of the query.
 *  _Bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_query_target_t target: The queryables that should be targeted by this get.
 *   z_owned_bytes_t *attachment: An optional attachment to the query.
 */
typedef struct {
    z_owned_bytes_t *payload;
    z_owned_encoding_t *encoding;
    z_query_consolidation_t consolidation;
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    _Bool is_express;
    z_query_target_t target;
    uint32_t timeout_ms;
    z_owned_bytes_t *attachment;
} z_get_options_t;

/**
 * Represents the configuration used to configure a read task started via :c:func:`zp_start_read_task`.
 */
typedef struct {
#if Z_FEATURE_MULTI_THREAD == 1
    z_task_attr_t *task_attributes;
#else
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
#endif
} zp_task_read_options_t;

/**
 * Represents the configuration used to configure a lease task started via :c:func:`zp_start_lease_task`.
 */
typedef struct {
#if Z_FEATURE_MULTI_THREAD == 1
    z_task_attr_t *task_attributes;
#else
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
#endif
} zp_task_lease_options_t;

/**
 * Represents the configuration used to configure a read operation started via :c:func:`zp_read`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_read_options_t;

/**
 * Represents the configuration used to configure a send keep alive operation started via :c:func:`zp_send_keep_alive`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_send_keep_alive_options_t;

/**
 * Represents the configuration used to configure a send join operation started via :c:func:`zp_send_join`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} zp_send_join_options_t;

/**
 * Represents the configuration used to configure a publisher upon declaration with :c:func:`z_declare_publisher`.
 *
 * Members:
 *   uint64_t timeout_ms: The maximum duration in ms the scouting can take.
 *   z_what_t what: Type of entities to scout for.
 */
typedef struct {
    uint32_t timeout_ms;
    z_what_t what;
} z_scout_options_t;

/**
 * QoS settings of a zenoh message.
 */
typedef _z_qos_t z_qos_t;

/**
 * Returns message priority.
 */
static inline z_priority_t z_qos_get_priority(z_qos_t qos) {
    z_priority_t ret = _z_n_qos_get_priority(qos);
    return ret == _Z_PRIORITY_CONTROL ? Z_PRIORITY_DEFAULT : ret;
}

/**
 * Returns message congestion control.
 */
static inline z_congestion_control_t z_qos_get_congestion_control(z_qos_t qos) {
    return _z_n_qos_get_congestion_control(qos);
}

/**
 * Returns message express flag. If set to true, the message is not batched to reduce the latency.
 */
static inline _Bool z_qos_get_express(z_qos_t qos) { return _z_n_qos_get_express(qos); }

/**
 * Returns default qos settings.
 */
static inline z_qos_t z_qos_default(void) { return _Z_N_QOS_DEFAULT; }

/**
 * Represents a data sample.
 *
 * A sample is the value associated to a given key-expression at a given point in time.
 *
 */
_Z_OWNED_TYPE_VALUE(_z_sample_t, sample)
_Z_LOANED_TYPE(_z_sample_t, sample)

/**
 * Represents the content of a `hello` message returned by a zenoh entity as a reply to a `scout` message.
 *
 * Members:
 *   z_whatami_t whatami: The kind of zenoh entity.
 *   z_loaned_slice_t* zid: The Zenoh ID of the scouted entity (empty if absent).
 *   z_loaned_string_array_t locators: The locators of the scouted entity.
 */
_Z_OWNED_TYPE_VALUE(_z_hello_t, hello)
_Z_LOANED_TYPE(_z_hello_t, hello)

/**
 * Represents the reply to a query.
 */
_Z_OWNED_TYPE_VALUE(_z_reply_t, reply)
_Z_LOANED_TYPE(_z_reply_t, reply)

/**
 * Represents an array of non null-terminated string.
 *
 * Operations over :c:type:`z_loaned_string_array_t` must be done using the provided functions:
 *
 *   - :c:func:`z_string_array_get`
 *   - :c:func:`z_string_array_len`
 *   - :c:func:`z_str_array_array_is_empty`
 */
_Z_OWNED_TYPE_VALUE(_z_string_svec_t, string_array)
_Z_LOANED_TYPE(_z_string_svec_t, string_array)
_Z_VIEW_TYPE(_z_string_svec_t, string_array)

const z_loaned_string_t *z_string_array_get(const z_loaned_string_array_t *a, size_t k);
size_t z_string_array_len(const z_loaned_string_array_t *a);
_Bool z_string_array_is_empty(const z_loaned_string_array_t *a);

typedef void (*z_dropper_handler_t)(void *arg);
typedef _z_data_handler_t z_data_handler_t;

typedef struct {
    void *context;
    z_data_handler_t call;
    z_dropper_handler_t drop;
} _z_closure_sample_t;
/**
 * Represents the sample closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   void *context: a pointer to an arbitrary state.
 *   z_data_handler_t call: `void *call(const struct z_sample_t*, const void *context)` is the callback function.
 *   z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_sample_t, closure_sample)
_Z_LOANED_TYPE(_z_closure_sample_t, closure_sample)

void z_closure_sample_call(const z_loaned_closure_sample_t *closure, const z_loaned_sample_t *sample);

typedef _z_queryable_handler_t z_queryable_handler_t;

typedef struct {
    void *context;
    z_queryable_handler_t call;
    z_dropper_handler_t drop;
} _z_closure_query_t;
/**
 * Represents the query callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   void *context: a pointer to an arbitrary state.
 *   _z_queryable_handler_t call: `void (*_z_queryable_handler_t)(z_query_t *query, void *arg)` is the
 * callback function.
 *   z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_query_t, closure_query)
_Z_LOANED_TYPE(_z_closure_query_t, closure_query)

void z_closure_query_call(const z_loaned_closure_query_t *closure, const z_loaned_query_t *query);

typedef _z_reply_handler_t z_reply_handler_t;

typedef struct {
    void *context;
    z_reply_handler_t call;
    z_dropper_handler_t drop;
} _z_closure_reply_t;
/**
 * Represents the query reply callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   void *context: a pointer to an arbitrary state.
 *   z_reply_handler_t call: `void (*_z_reply_handler_t)(_z_reply_t *reply, void *arg)` is the
 * callback function.
 *   z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_reply_t, closure_reply)
_Z_LOANED_TYPE(_z_closure_reply_t, closure_reply)

void z_closure_reply_call(const z_loaned_closure_reply_t *closure, const z_loaned_reply_t *reply);

typedef void (*z_loaned_hello_handler_t)(const z_loaned_hello_t *hello, void *arg);

typedef struct {
    void *context;
    z_loaned_hello_handler_t call;
    z_dropper_handler_t drop;
} _z_closure_hello_t;
/**
 * Represents the Zenoh ID callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   void *context: a pointer to an arbitrary state.
 *   z_loaned_hello_handler_t call: `void (*z_loaned_hello_handler_t)(const z_loaned_hello_t *hello, void *arg)` is the
 * callback function.
 *   z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_hello_t, closure_hello)
_Z_LOANED_TYPE(_z_closure_hello_t, closure_hello)

void z_closure_hello_call(const z_loaned_closure_hello_t *closure, const z_loaned_hello_t *hello);

typedef void (*z_id_handler_t)(const z_id_t *id, void *arg);

typedef struct {
    void *context;
    z_id_handler_t call;
    z_dropper_handler_t drop;
} _z_closure_zid_t;
/**
 * Represents the Zenoh ID callback closure.
 *
 * A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Members:
 *   void *context: a pointer to an arbitrary state.
 *   z_id_handler_t call: `void (*z_id_handler_t)(const z_id_t *id, void *arg)` is the callback function.
 *   z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_zid_t, closure_zid)
_Z_LOANED_TYPE(_z_closure_zid_t, closure_zid)

void z_closure_zid_call(const z_loaned_closure_zid_t *closure, const z_id_t *id);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_API_TYPES_H */
