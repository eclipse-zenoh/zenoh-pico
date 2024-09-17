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
_Z_VIEW_TYPE(_z_slice_t, slice)

/**
 * Represents a container for slices.
 */
_Z_OWNED_TYPE_VALUE(_z_bytes_t, bytes)

/**
 * Represents a writer for serialized data.
 */
typedef _z_bytes_iterator_writer_t z_bytes_writer_t;

/**
 * An iterator over multi-element serialized data.
 */
typedef _z_bytes_iterator_t z_bytes_iterator_t;

/**
 * A reader for serialized data.
 */
typedef _z_bytes_iterator_t z_bytes_reader_t;

/**
 * An iterator over slices of serialized data.
 */
typedef struct {
    const _z_bytes_t *_bytes;
    size_t _slice_idx;
} z_bytes_slice_iterator_t;

/**
 * Represents a string without null-terminator.
 *
 * Members:
 *   size_t len: The length of the string.
 *   const char *val: A pointer to the string.
 */
_Z_OWNED_TYPE_VALUE(_z_string_t, string)
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

/**
 * Represents a Zenoh Session.
 */
_Z_OWNED_TYPE_RC(_z_session_rc_t, session)

/**
 * Represents a Zenoh Subscriber entity.
 *
 * Members are private and operations must be done using the provided functions:
 *
 *   - :c:func:`z_declare_subscriber`
 *   - :c:func:`z_undeclare_subscriber`
 */
_Z_OWNED_TYPE_VALUE(_z_subscriber_t, subscriber)

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

/**
 * Represents a Zenoh Queryable entity.
 *
 * Members are private and operations must be done using the provided functions:
 *
 *   - :c:func:`z_declare_queryable`
 *   - :c:func:`z_undeclare_queryable`
 */
_Z_OWNED_TYPE_VALUE(_z_queryable_t, queryable)

/**
 * Represents a Zenoh Query entity, received by Zenoh Queryable entities.
 *
 */
_Z_OWNED_TYPE_RC(_z_query_rc_t, query)

/**
 * Represents the encoding of a payload, in a MIME-like format.
 *
 * Members:
 *   uint16_t prefix: The integer prefix of this encoding.
 *   z_loaned_slice_t* suffix: The suffix of this encoding. It MUST be a valid UTF-8 string.
 */
_Z_OWNED_TYPE_VALUE(_z_encoding_t, encoding)

/**
 * Represents a Zenoh reply error.
 *
 * Members:
 *   z_loaned_encoding_t encoding: The encoding of the error `payload`.
 *   z_loaned_bytes_t* payload: The payload of this zenoh reply error.
 */
_Z_OWNED_TYPE_VALUE(_z_value_t, reply_err)

/**
 * Represents the configuration used to configure a subscriber upon declaration :c:func:`z_declare_subscriber`.
 *.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} z_subscriber_options_t;

/**
 * Represents the configuration used to configure a zenoh upon opening :c:func:`z_open`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} z_open_options_t;

/**
 * Represents the configuration used to configure a zenoh upon closing :c:func:`z_close`.
 */
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} z_close_options_t;

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
 *   z_owned_encoding_t *encoding: Default encoding for messages put by this publisher.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing messages from this
 * publisher.
 *   z_priority_t priority: The priority of messages issued by this publisher.
 *   bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   (unstable) z_reliability_t reliability: The reliability that should be used to transmit the data.
 */
typedef struct {
    z_moved_encoding_t *encoding;
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    bool is_express;
#ifdef Z_FEATURE_UNSTABLE_API
    z_reliability_t reliability;
#endif
} z_publisher_options_t;

/**
 * Represents the configuration used to configure a queryable upon declaration :c:func:`z_declare_queryable`.
 *
 * Members:
 *   bool complete: The completeness of the queryable.
 */
typedef struct {
    bool complete;
} z_queryable_options_t;

/**
 * Represents the configuration used to configure a query reply sent via :c:func:`z_query_reply.
 *
 * Members:
 *   z_moved_encoding_t* encoding: The encoding of the payload.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_moved_bytes_t* attachment: An optional attachment to the response.
 */
typedef struct {
    z_moved_encoding_t *encoding;
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    z_timestamp_t *timestamp;
    bool is_express;
    z_moved_bytes_t *attachment;
} z_query_reply_options_t;

/**
 * Represents the configuration used to configure a query reply delete sent via :c:func:`z_query_reply_del.
 *
 * Members:
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_moved_bytes_t* attachment: An optional attachment to the response.
 */
typedef struct {
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    z_timestamp_t *timestamp;
    bool is_express;
    z_moved_bytes_t *attachment;
} z_query_reply_del_options_t;

/**
 * Represents the configuration used to configure a query reply error sent via :c:func:`z_query_reply_err.
 *
 * Members:
 *   z_moved_encoding_t* encoding: The encoding of the payload.
 */
typedef struct {
    z_moved_encoding_t *encoding;
} z_query_reply_err_options_t;

/**
 * Represents the configuration used to configure a put operation sent via via :c:func:`z_put`.
 *
 * Members:
 *   z_moved_encoding_t* encoding: The encoding of the payload.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_moved_bytes_t* attachment: An optional attachment to the publication.
 *   (unstable) z_reliability_t reliability: The reliability that should be used to transmit the data.
 */
typedef struct {
    z_moved_encoding_t *encoding;
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    z_timestamp_t *timestamp;
    bool is_express;
    z_moved_bytes_t *attachment;
#ifdef Z_FEATURE_UNSTABLE_API
    z_reliability_t reliability;
#endif
} z_put_options_t;

/**
 * Represents the configuration used to configure a delete operation sent via :c:func:`z_delete`.
 *
 * Members:
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when router.
 *   bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   (unstable) z_reliability_t reliability: The reliability that should be used to transmit the data.
 */
typedef struct {
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    bool is_express;
    z_timestamp_t *timestamp;
#ifdef Z_FEATURE_UNSTABLE_API
    z_reliability_t reliability;
#endif
} z_delete_options_t;

/**
 * Represents the configuration used to configure a put operation by a previously declared publisher,
 * sent via :c:func:`z_publisher_put`.
 *
 * Members:
 *   z_moved_encoding_t* encoding: The encoding of the payload.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   z_moved_bytes_t* attachment: An optional attachment to the publication.
 */
typedef struct {
    z_moved_encoding_t *encoding;
    z_timestamp_t *timestamp;
    z_moved_bytes_t *attachment;
} z_publisher_put_options_t;

/**
 * Represents the configuration used to configure a delete operation by a previously declared publisher,
 * sent via :c:func:`z_publisher_delete`.
 *
 * Members:
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 */
typedef struct {
    z_timestamp_t *timestamp;
} z_publisher_delete_options_t;

/**
 * Represents the configuration used to configure a get operation sent via :c:func:`z_get`.
 *
 * Members:
 *   z_moved_bytes_t* payload: The payload to include in the query.
 *   z_moved_encoding_t* encoding: Payload encoding.
 *   z_query_consolidation_t consolidation: The replies consolidation strategy to apply on replies.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing the query.
 *   z_priority_t priority: The priority of the query.
 *  bool is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_query_target_t target: The queryables that should be targeted by this get.
 *   z_moved_bytes_t* attachment: An optional attachment to the query.
 */
typedef struct {
    z_moved_bytes_t *payload;
    z_moved_encoding_t *encoding;
    z_query_consolidation_t consolidation;
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    bool is_express;
    z_query_target_t target;
    uint64_t timeout_ms;
    z_moved_bytes_t *attachment;
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
 * Represents a data sample.
 *
 * A sample is the value associated to a given key-expression at a given point in time.
 *
 */
_Z_OWNED_TYPE_VALUE(_z_sample_t, sample)

/**
 * Represents the content of a `hello` message returned by a zenoh entity as a reply to a `scout` message.
 *
 * Members:
 *   z_whatami_t whatami: The kind of zenoh entity.
 *   z_loaned_slice_t* zid: The Zenoh ID of the scouted entity (empty if absent).
 *   z_loaned_string_array_t locators: The locators of the scouted entity.
 */
_Z_OWNED_TYPE_VALUE(_z_hello_t, hello)

/**
 * Represents the reply to a query.
 */
_Z_OWNED_TYPE_VALUE(_z_reply_t, reply)

/**
 * Represents an array of non null-terminated string.
 *
 * Operations over :c:type:`z_loaned_string_array_t` must be done using the provided functions:
 *
 *   - :c:func:`z_string_array_new`
 *   - :c:func:`z_string_array_push_by_alias`
 *   - :c:func:`z_string_array_push_by_copy`
 *   - :c:func:`z_string_array_get`
 *   - :c:func:`z_string_array_len`
 *   - :c:func:`z_str_array_array_is_empty`
 */
_Z_OWNED_TYPE_VALUE(_z_string_svec_t, string_array)
_Z_VIEW_TYPE(_z_string_svec_t, string_array)

void z_string_array_new(z_owned_string_array_t *a);
size_t z_string_array_push_by_alias(z_loaned_string_array_t *a, const z_loaned_string_t *value);
size_t z_string_array_push_by_copy(z_loaned_string_array_t *a, const z_loaned_string_t *value);
const z_loaned_string_t *z_string_array_get(const z_loaned_string_array_t *a, size_t k);
size_t z_string_array_len(const z_loaned_string_array_t *a);
bool z_string_array_is_empty(const z_loaned_string_array_t *a);

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
 *   z_data_handler_t call: `void *call(z_loaned_sample_t*, const void *context)` is the callback function.
 *   z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_sample_t, closure_sample)

void z_closure_sample_call(const z_loaned_closure_sample_t *closure, z_loaned_sample_t *sample);

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
 *   _z_queryable_handler_t call: `void (*_z_queryable_handler_t)(z_loaned_query_t *query, void *arg)` is the
 * callback function.
 *   z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_query_t, closure_query)

void z_closure_query_call(const z_loaned_closure_query_t *closure, z_loaned_query_t *query);

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
 *   z_reply_handler_t call: `void (*_z_reply_handler_t)(z_loaned_reply_t *reply, void *arg)` is the
 * callback function.
 *   z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_reply_t, closure_reply)

void z_closure_reply_call(const z_loaned_closure_reply_t *closure, z_loaned_reply_t *reply);

typedef void (*z_loaned_hello_handler_t)(z_loaned_hello_t *hello, void *arg);

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
 *   z_loaned_hello_handler_t call: `void (*z_loaned_hello_handler_t)(z_loaned_hello_t *hello, void *arg)` is the
 * callback function.
 *   z_dropper_handler_t drop: `void *drop(void*)` allows the callback's state to be freed.
 *   void *context: a pointer to an arbitrary state.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_hello_t, closure_hello)

void z_closure_hello_call(const z_loaned_closure_hello_t *closure, z_loaned_hello_t *hello);

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

void z_closure_zid_call(const z_loaned_closure_zid_t *closure, const z_id_t *id);

/**
 * Default encoding values used by Zenoh.
 *
 * An encoding has a similar role to Content-type in HTTP: it indicates, when present, how data should be interpreted by
 * the application.
 *
 * Please note the Zenoh protocol does not impose any encoding value nor it operates on it.
 * It can be seen as some optional metadata that is carried over by Zenoh in such a way the application may perform
 * different operations depending on the encoding value.
 *
 * A set of associated constants are provided to cover the most common encodings for user convenience.
 * This is particularly useful in helping Zenoh to perform additional wire-level optimizations.
 *
 * Register your encoding metadata from a string with :c:func:`z_encoding_from_str`. To get the optimization, you need
 * Z_FEATURE_ENCODING_VALUES to 1 and your string should follow the format: "<constant>;<optional additional data>"
 *
 * E.g: "text/plain;utf8"
 *
 * Or you can set the value to the constants directly with this list of constants:
 */

#if Z_FEATURE_ENCODING_VALUES == 1
// - Below are Primitives types, supported in all Zenoh bindings
// Just some bytes.
//
// Constant alias for string: `"zenoh/bytes"`.
//
// Usually used for types: `uint8_t[]`.
extern const z_owned_encoding_t ENCODING_ZENOH_BYTES;

// A VLE-encoded signed little-endian 8bit integer. Binary representation uses two's complement.
//// - Primitives types supported in all Zenoh bindings
// Constant alias for string: `"zenoh/int8"`.
//
// Usually used for types: `int8_t`.
extern const z_owned_encoding_t ENCODING_ZENOH_INT8;

// A VLE-encoded signed little-endian 16bit integer. Binary representation uses two's complement.
//
// Constant alias for string: `"zenoh/int16"`.
//
// Usually used for types: `int16_t`.
extern const z_owned_encoding_t ENCODING_ZENOH_INT16;

// A VLE-encoded signed little-endian 32bit integer. Binary representation uses two's complement.
//
// Constant alias for string: `"zenoh/int32"`.
//
// Usually used for types: `int32_t`.
extern const z_owned_encoding_t ENCODING_ZENOH_INT32;

// A VLE-encoded signed little-endian 64bit integer. Binary representation uses two's complement.
//
// Constant alias for string: `"zenoh/int64"`.
//
// Usually used for types: `int64_t`.
extern const z_owned_encoding_t ENCODING_ZENOH_INT64;

// A VLE-encoded signed little-endian 128bit integer. Binary representation uses two's complement.
//
// Constant alias for string: `"zenoh/int128"`.
extern const z_owned_encoding_t ENCODING_ZENOH_INT128;

// A VLE-encoded unsigned little-endian 8bit integer.
//
// Constant alias for string: `"zenoh/uint8"`.
//
// Usually used for types: `uint8_t`.
extern const z_owned_encoding_t ENCODING_ZENOH_UINT8;

// A VLE-encoded unsigned little-endian 16bit integer.
//
// Constant alias for string: `"zenoh/uint16"`.
//
// Usually used for types: `uint16_t`.
extern const z_owned_encoding_t ENCODING_ZENOH_UINT16;
// A VLE-encoded unsigned little-endian 32bit integer.
//
// Constant alias for string: `"zenoh/uint32"`.
//
// Usually used for types: `uint32_t`.
extern const z_owned_encoding_t ENCODING_ZENOH_UINT32;

// A VLE-encoded unsigned little-endian 64bit integer.
//
// Constant alias for string: `"zenoh/uint64"`.
//
// Usually used for types: `uint64_t`.
extern const z_owned_encoding_t ENCODING_ZENOH_UINT64;

// A VLE-encoded unsigned little-endian 128bit integer.
//
// Constant alias for string: `"zenoh/uint128"`.
extern const z_owned_encoding_t ENCODING_ZENOH_UINT128;

// A VLE-encoded 32bit float. Binary representation uses *IEEE 754-2008* *binary32* .
//
// Constant alias for string: `"zenoh/float32"`.
//
// Usually used for types: `float`.
extern const z_owned_encoding_t ENCODING_ZENOH_FLOAT32;

// A VLE-encoded 64bit float. Binary representation uses *IEEE 754-2008* *binary64*.
//
// Constant alias for string: `"zenoh/float64"`.
//
// Usually used for types: `double`.
extern const z_owned_encoding_t ENCODING_ZENOH_FLOAT64;

// A boolean. `0` is `false`, `1` is `true`. Other values are invalid.
//
// Constant alias for string: `"zenoh/bool"`.
//
// Usually used for types: `bool`.
extern const z_owned_encoding_t ENCODING_ZENOH_BOOL;

// A UTF-8 string.
//
// Constant alias for string: `"zenoh/string"`.
//
// Usually used for types: `char[]`.
extern const z_owned_encoding_t ENCODING_ZENOH_STRING;
// A zenoh error.
//
// Constant alias for string: `"zenoh/error"`.
//
// Usually used for types: `z_reply_err_t`.
extern const z_owned_encoding_t ENCODING_ZENOH_ERROR;

// - Below are Advanced types, may be supported in some of the Zenoh bindings.
// An application-specific stream of bytes.
//
// Constant alias for string: `"application/octet-stream"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_OCTET_STREAM;

// A textual file.
//
// Constant alias for string: `"text/plain"`.
extern const z_owned_encoding_t ENCODING_TEXT_PLAIN;

// JSON data intended to be consumed by an application.
//
// Constant alias for string: `"application/json"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JSON;

// JSON data intended to be human readable.
//
// Constant alias for string: `"text/json"`.
extern const z_owned_encoding_t ENCODING_TEXT_JSON;

// A Common Data Representation (CDR)-encoded data.
//
// Constant alias for string: `"application/cdr"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_CDR;

// A Concise Binary Object Representation (CBOR)-encoded data.
//
// Constant alias for string: `"application/cbor"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_CBOR;

// YAML data intended to be consumed by an application.
//
// Constant alias for string: `"application/yaml"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_YAML;

// YAML data intended to be human readable.
//
// Constant alias for string: `"text/yaml"`.
extern const z_owned_encoding_t ENCODING_TEXT_YAML;

// JSON5 encoded data that are human readable.
//
// Constant alias for string: `"text/json5"`.
extern const z_owned_encoding_t ENCODING_TEXT_JSON5;

// A Python object serialized using [pickle](https://docs.python.org/3/library/pickle.html).
//
// Constant alias for string: `"application/python-serialized-object"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_PYTHON_SERIALIZED_OBJECT;

// An application-specific protobuf-encoded data.
//
// Constant alias for string: `"application/protobuf"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_PROTOBUF;

// A Java serialized object.
//
// Constant alias for string: `"application/java-serialized-object"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JAVA_SERIALIZED_OBJECT;

// An [openmetrics](https://github.com/OpenObservability/OpenMetrics) data, common used by
// [Prometheus](https://prometheus.io/).
//
// Constant alias for string: `"application/openmetrics-text"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_OPENMETRICS_TEXT;

// A Portable Network Graphics (PNG) image.
//
// Constant alias for string: `"image/png"`.
extern const z_owned_encoding_t ENCODING_IMAGE_PNG;

// A Joint Photographic Experts Group (JPEG) image.
//
// Constant alias for string: `"image/jpeg"`.
extern const z_owned_encoding_t ENCODING_IMAGE_JPEG;

// A Graphics Interchange Format (GIF) image.
//
// Constant alias for string: `"image/gif"`.
extern const z_owned_encoding_t ENCODING_IMAGE_GIF;

// A BitMap (BMP) image.
//
// Constant alias for string: `"image/bmp"`.
extern const z_owned_encoding_t ENCODING_IMAGE_BMP;

// A Web Portable (WebP) image.
//
//  Constant alias for string: `"image/webp"`.
extern const z_owned_encoding_t ENCODING_IMAGE_WEBP;

// An XML file intended to be consumed by an application..
//
// Constant alias for string: `"application/xml"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_XML;

// An encoded a list of tuples, each consisting of a name and a value.
//
// Constant alias for string: `"application/x-www-form-urlencoded"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_X_WWW_FORM_URLENCODED;

// An HTML file.
//
// Constant alias for string: `"text/html"`.
extern const z_owned_encoding_t ENCODING_TEXT_HTML;

// An XML file that is human readable.
//
// Constant alias for string: `"text/xml"`.
extern const z_owned_encoding_t ENCODING_TEXT_XML;

// A CSS file.
//
// Constant alias for string: `"text/css"`.
extern const z_owned_encoding_t ENCODING_TEXT_CSS;

// A JavaScript file.
//
// Constant alias for string: `"text/javascript"`.
extern const z_owned_encoding_t ENCODING_TEXT_JAVASCRIPT;

// A MarkDown file.
//
// Constant alias for string: `"text/markdown"`.
extern const z_owned_encoding_t ENCODING_TEXT_MARKDOWN;

// A CSV file.
//
// Constant alias for string: `"text/csv"`.
extern const z_owned_encoding_t ENCODING_TEXT_CSV;

// An application-specific SQL query.
//
// Constant alias for string: `"application/sql"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_SQL;

// Constrained Application Protocol (CoAP) data intended for CoAP-to-HTTP and HTTP-to-CoAP proxies.
//
// Constant alias for string: `"application/coap-payload"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_COAP_PAYLOAD;

// Defines a JSON document structure for expressing a sequence of operations to apply to a JSON document.
//
// Constant alias for string: `"application/json-patch+json"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JSON_PATCH_JSON;

// A JSON text sequence consists of any number of JSON texts, all encoded in UTF-8.
//
// Constant alias for string: `"application/json-seq"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JSON_SEQ;

// A JSONPath defines a string syntax for selecting and extracting JSON values from within a given JSON value.
//
// Constant alias for string: `"application/jsonpath"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JSONPATH;

// A JSON Web Token (JWT).
//
// Constant alias for string: `"application/jwt"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JWT;

// An application-specific MPEG-4 encoded data, either audio or video.
//
// Constant alias for string: `"application/mp4"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_MP4;

// A SOAP 1.2 message serialized as XML 1.0.
//
// Constant alias for string: `"application/soap+xml"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_SOAP_XML;

// A YANG-encoded data commonly used by the Network Configuration Protocol (NETCONF).
//
// Constant alias for string: `"application/yang"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_YANG;

// A MPEG-4 Advanced Audio Coding (AAC) media.
//
// Constant alias for string: `"audio/aac"`.
extern const z_owned_encoding_t ENCODING_AUDIO_AAC;

// A Free Lossless Audio Codec (FLAC) media.
//
// Constant alias for string: `"audio/flac"`.
extern const z_owned_encoding_t ENCODING_AUDIO_FLAC;

// An audio codec defined in MPEG-1, MPEG-2, MPEG-4, or registered at the MP4 registration authority.
//
// Constant alias for string: `"audio/mp4"`.
extern const z_owned_encoding_t ENCODING_AUDIO_MP4;

// An Ogg-encapsulated audio stream.
//
// Constant alias for string: `"audio/ogg"`.
extern const z_owned_encoding_t ENCODING_AUDIO_OGG;

// A Vorbis-encoded audio stream.
//
// Constant alias for string: `"audio/vorbis"`.
extern const z_owned_encoding_t ENCODING_AUDIO_VORBIS;

// A h261-encoded video stream.
//
// Constant alias for string: `"video/h261"`.
extern const z_owned_encoding_t ENCODING_VIDEO_H261;

// A h263-encoded video stream.
//
// Constant alias for string: `"video/h263"`.
extern const z_owned_encoding_t ENCODING_VIDEO_H263;

// A h264-encoded video stream.
//
// Constant alias for string: `"video/h264"`.
extern const z_owned_encoding_t ENCODING_VIDEO_H264;

// A h265-encoded video stream.
//
// Constant alias for string: `"video/h265"`.
extern const z_owned_encoding_t ENCODING_VIDEO_H265;

// A h266-encoded video stream.
//
// Constant alias for string: `"video/h266"`.
extern const z_owned_encoding_t ENCODING_VIDEO_H266;

// A video codec defined in MPEG-1, MPEG-2, MPEG-4, or registered at the MP4 registration authority.
//
// Constant alias for string: `"video/mp4"`.
extern const z_owned_encoding_t ENCODING_VIDEO_MP4;

// An Ogg-encapsulated video stream.
//
// Constant alias for string: `"video/ogg"`.
extern const z_owned_encoding_t ENCODING_VIDEO_OGG;

// An uncompressed, studio-quality video stream.
//
// Constant alias for string: `"video/raw"`.
extern const z_owned_encoding_t ENCODING_VIDEO_RAW;

// A VP8-encoded video stream.
//
// Constant alias for string: `"video/vp8"`.
extern const z_owned_encoding_t ENCODING_VIDEO_VP8;

// A VP9-encoded video stream.
//
// Constant alias for string: `"video/vp9"`.
extern const z_owned_encoding_t ENCODING_VIDEO_VP9;
#endif

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_API_TYPES_H */
