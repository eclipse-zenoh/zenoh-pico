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
#include "zenoh-pico/net/matching.h"
#include "zenoh-pico/net/publish.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/net/reply.h"
#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a Zenoh ID.
 *
 * In general, valid Zenoh IDs are LSB-first 128bit unsigned and non-zero integers.
 *
 * Members:
 *   uint8_t id[16]: The array containing the 16 octets of a Zenoh ID.
 */
typedef _z_id_t z_id_t;

/**
 *  Represents an ID globally identifying an entity in a Zenoh system.
 */
typedef _z_entity_global_id_t z_entity_global_id_t;

/*
 * Represents timestamp value in Zenoh
 */
typedef _z_timestamp_t z_timestamp_t;

/**
 * Represents an array of bytes.
 */
_Z_OWNED_TYPE_VALUE(_z_slice_t, slice)
_Z_VIEW_TYPE(_z_slice_t, slice)

/**
 * Represents a container for slices.
 */
_Z_OWNED_TYPE_VALUE(_z_bytes_t, bytes)

/**
 * Represents a writer for data.
 */
_Z_OWNED_TYPE_VALUE(_z_bytes_writer_t, bytes_writer)

/**
 * A reader for data.
 */
typedef _z_bytes_reader_t z_bytes_reader_t;

/**
 * An iterator over slices of data.
 */
typedef struct {
    const _z_bytes_t *_bytes;
    size_t _slice_idx;
} z_bytes_slice_iterator_t;

/**
 * Represents a string without null-terminator.
 */
_Z_OWNED_TYPE_VALUE(_z_string_t, string)
_Z_VIEW_TYPE(_z_string_t, string)

/**
 * Represents a key expression in Zenoh.
 */
_Z_OWNED_TYPE_VALUE(_z_keyexpr_t, keyexpr)
_Z_VIEW_TYPE(_z_keyexpr_t, keyexpr)

/**
 * Represents a Zenoh configuration, used to configure Zenoh sessions upon opening.
 */
_Z_OWNED_TYPE_VALUE(_z_config_t, config)

/**
 * Represents a Zenoh Session.
 */
_Z_OWNED_TYPE_RC(_z_session_rc_t, session)

/**
 * Represents a Zenoh Subscriber entity.
 */
_Z_OWNED_TYPE_VALUE(_z_subscriber_t, subscriber)

/**
 * Represents a Zenoh Publisher entity.
 */
_Z_OWNED_TYPE_VALUE(_z_publisher_t, publisher)

/**
 * Represents a Zenoh Querier entity.
 */
_Z_OWNED_TYPE_VALUE(_z_querier_t, querier)

/**
 * Represents a Zenoh Matching listener entity.
 */
_Z_OWNED_TYPE_VALUE(_z_matching_listener_t, matching_listener)

/**
 * Represents a Zenoh Queryable entity.
 */
_Z_OWNED_TYPE_VALUE(_z_queryable_t, queryable)

/**
 * Represents a Zenoh Query entity, received by Zenoh Queryable entities.
 */
_Z_OWNED_TYPE_RC(_z_query_rc_t, query)

/**
 * Represents the encoding of a payload, in a MIME-like format.
 */
_Z_OWNED_TYPE_VALUE(_z_encoding_t, encoding)

/**
 * Represents a Zenoh reply error.
 */
_Z_OWNED_TYPE_VALUE(_z_value_t, reply_err)

/**
 * Represents sample source information.
 */
_Z_OWNED_TYPE_VALUE(_z_source_info_t, source_info)

/**
 * A struct that indicates if there exist Subscribers matching the Publisher's key expression or Queryables matching
 * Querier's key expression and target.
 * Members:
 *   bool matching: true if there exist matching Zenoh entities, false otherwise.
 */
typedef _z_matching_status_t z_matching_status_t;

/**
 * Represents the configuration used to configure a subscriber upon declaration :c:func:`z_declare_subscriber`.
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
 *   z_moved_encoding_t *encoding: Default encoding for messages put by this publisher.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing messages from this
 *     publisher.
 *   z_priority_t priority: The priority of messages issued by this publisher.
 *   bool is_express: If ``true``, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_reliability_t reliability: The reliability that should be used to transmit the data (unstable).
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
 * Options passed to the :c:func:`z_declare_querier()` function.
 *
 * Members:
 *   z_moved_encoding_t *encoding: Default encoding for values sent by this querier.
 *   z_query_target_t target: The Queryables that should be target of the querier queries.
 *   z_query_consolidation_t consolidation: The replies consolidation strategy to apply on replies to the querier
 *    queries.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing the querier queries.
 *   bool is_express: If set to ``true``, the querier queries will not be batched. This usually has a positive impact on
 * 	   latency but negative impact on throughput.
 *   z_priority_t priority: The priority of the querier queries.
 *   uint64_t timeout_ms: The timeout for the querier queries in milliseconds. 0 means default query timeout from zenoh
 *     configuration.
 */
typedef struct z_querier_options_t {
    z_moved_encoding_t *encoding;
    z_query_target_t target;
    z_query_consolidation_t consolidation;
    z_congestion_control_t congestion_control;
    bool is_express;
    z_priority_t priority;
    uint64_t timeout_ms;
} z_querier_options_t;

/**
 * Options passed to the :c:func:`z_querier_get()` function.
 *
 * Members:
 *   z_moved_bytes_t *payload: An optional payload to attach to the query.
 *   z_moved_encoding_t *encoding: An optional encoding of the query payload and or attachment.
 *   z_moved_bytes_t *attachment: An optional attachment to attach to the query.
 */
typedef struct z_querier_get_options_t {
    z_moved_bytes_t *payload;
    z_moved_encoding_t *encoding;
    z_moved_bytes_t *attachment;
} z_querier_get_options_t;

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
 * Represents the configuration used to configure a query reply sent via :c:func:`z_query_reply`.
 *
 * Members:
 *   z_moved_encoding_t* encoding: The encoding of the payload.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   bool is_express: If ``true``, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
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
 * Represents the configuration used to configure a query reply delete sent via :c:func:`z_query_reply_del`.
 *
 * Members:
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   bool is_express: If ``true``, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
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
 * Represents the configuration used to configure a query reply error sent via :c:func:`z_query_reply_err`.
 *
 * Members:
 *   z_moved_encoding_t* encoding: The encoding of the payload.
 */
typedef struct {
    z_moved_encoding_t *encoding;
} z_query_reply_err_options_t;

/**
 * Represents the configuration used to configure a put operation sent via :c:func:`z_put`.
 *
 * Members:
 *   z_moved_encoding_t* encoding: The encoding of the payload.
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when routed.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   bool is_express: If ``true``, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_moved_bytes_t* attachment: An optional attachment to the publication.
 *   z_reliability_t reliability: The reliability that should be used to transmit the data (unstable).
 *   z_moved_source_info_t* source_info: The source info for the message (unstable).
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
    z_moved_source_info_t *source_info;
#endif
} z_put_options_t;

/**
 * Represents the configuration used to configure a delete operation sent via :c:func:`z_delete`.
 *
 * Members:
 *   z_congestion_control_t congestion_control: The congestion control to apply when routing this message.
 *   z_priority_t priority: The priority of this message when router.
 *   bool is_express: If ``true``, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   z_reliability_t reliability: The reliability that should be used to transmit the data (unstable).
 *   z_moved_source_info_t* source_info: The source info for the message (unstable).
 */
typedef struct {
    z_congestion_control_t congestion_control;
    z_priority_t priority;
    bool is_express;
    z_timestamp_t *timestamp;
#ifdef Z_FEATURE_UNSTABLE_API
    z_reliability_t reliability;
    z_moved_source_info_t *source_info;
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
 *   z_moved_source_info_t* source_info: The source info for the message (unstable).
 */
typedef struct {
    z_moved_encoding_t *encoding;
    z_timestamp_t *timestamp;
    z_moved_bytes_t *attachment;
#ifdef Z_FEATURE_UNSTABLE_API
    z_moved_source_info_t *source_info;
#endif
} z_publisher_put_options_t;

/**
 * Represents the configuration used to configure a delete operation by a previously declared publisher,
 * sent via :c:func:`z_publisher_delete`.
 *
 * Members:
 *   z_timestamp_t *timestamp: The API level timestamp (e.g. of the data when it was created).
 *   z_moved_source_info_t* source_info: The source info for the message (unstable).
 */
typedef struct {
    z_timestamp_t *timestamp;
#ifdef Z_FEATURE_UNSTABLE_API
    z_moved_source_info_t *source_info;
#endif
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
 *   bool is_express: If ``true``, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
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
 */
_Z_OWNED_TYPE_VALUE(_z_sample_t, sample)

/**
 * Represents the content of a `hello` message returned by a zenoh entity as a reply to a `scout` message.
 */
_Z_OWNED_TYPE_VALUE(_z_hello_t, hello)

/**
 * Represents the reply to a query.
 */
_Z_OWNED_TYPE_VALUE(_z_reply_t, reply)

/**
 * Represents an array of non null-terminated string.
 */
_Z_OWNED_TYPE_VALUE(_z_string_svec_t, string_array)

typedef _z_drop_handler_t z_closure_drop_callback_t;
typedef _z_closure_sample_callback_t z_closure_sample_callback_t;

typedef struct {
    void *context;
    z_closure_sample_callback_t call;
    z_closure_drop_callback_t drop;
} _z_closure_sample_t;

/**
 * Represents the sample closure.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_sample_t, closure_sample)

typedef _z_closure_query_callback_t z_closure_query_callback_t;

typedef struct {
    void *context;
    z_closure_query_callback_t call;
    z_closure_drop_callback_t drop;
} _z_closure_query_t;

/**
 * Represents the query callback closure.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_query_t, closure_query)

typedef _z_closure_reply_callback_t z_closure_reply_callback_t;

typedef struct {
    void *context;
    z_closure_reply_callback_t call;
    z_closure_drop_callback_t drop;
} _z_closure_reply_t;

/**
 * Represents the query reply callback closure.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_reply_t, closure_reply)

typedef void (*z_closure_hello_callback_t)(z_loaned_hello_t *hello, void *arg);

typedef struct {
    void *context;
    z_closure_hello_callback_t call;
    z_closure_drop_callback_t drop;
} _z_closure_hello_t;

/**
 * Represents the Zenoh ID callback closure.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_hello_t, closure_hello)

typedef void (*z_closure_zid_callback_t)(const z_id_t *id, void *arg);

typedef struct {
    void *context;
    z_closure_zid_callback_t call;
    z_closure_drop_callback_t drop;
} _z_closure_zid_t;

/**
 * Represents the Zenoh ID callback closure.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_zid_t, closure_zid)

typedef _z_closure_matching_status_callback_t z_closure_matching_status_callback_t;
typedef _z_closure_matching_status_t z_closure_matching_status_t;
/**
 * Represents the matching status callback closure.
 */
_Z_OWNED_TYPE_VALUE(_z_closure_matching_status_t, closure_matching_status)

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_API_TYPES_H */
