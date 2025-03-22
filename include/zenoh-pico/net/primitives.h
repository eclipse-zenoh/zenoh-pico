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

#ifndef INCLUDE_ZENOH_PICO_NET_PRIMITIVES_H
#define INCLUDE_ZENOH_PICO_NET_PRIMITIVES_H

#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/net/publish.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"
#include "zenoh-pico/protocol/core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------- Declaration Helpers --------------*/
z_result_t _z_send_declare(_z_session_t *zn, const _z_network_message_t *n_msg);
z_result_t _z_send_undeclare(_z_session_t *zn, const _z_network_message_t *n_msg);

/*------------------ Discovery ------------------*/

/**
 * Scout for routers and/or peers.
 *
 * Parameters:
 *     what: A what bitmask of zenoh entities kind to scout for.
 *     zid: The ZenohID of the scouting origin.
 *     locator: The locator where to scout.
 *     timeout: The time that should be spent scouting before returning the results.
 */
void _z_scout(const z_what_t what, const _z_id_t zid, _z_string_t *locator, const uint32_t timeout,
              _z_closure_hello_callback_t callback, void *arg_call, _z_drop_handler_t dropper, void *arg_drop);

/*------------------ Declarations ------------------*/

/**
 * Associate a numerical id with the given resource key.
 *
 * This numerical id will be used on the network to save bandwidth and
 * ease the retrieval of the concerned resource in the routing tables.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     keyexpr: The resource key to map to a numerical id. The callee gets
 *             the ownership of any allocated value.
 *
 * Returns:
 *     A numerical id of the declared resource.
 */
uint16_t _z_declare_resource(_z_session_t *zn, const _z_keyexpr_t *keyexpr);

/**
 * Associate a numerical id with the given resource key.
 *
 * This numerical id will be used on the network to save bandwidth and
 * ease the retrieval of the concerned resource in the routing tables.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     rid: The numerical id of the resource to undeclare.
 * Returns:
 *    0 if success, or a negative value identifying the error.
 */
z_result_t _z_undeclare_resource(_z_session_t *zn, uint16_t rid);

/**
 * Declare keyexpr if it is necessary and allowed.
 * Returns updated keyexpr.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     keyexpr: The resource key to declare.
 * Returns:
 *     Updated keyexpr.
 */
_z_keyexpr_t _z_update_keyexpr_to_declared(_z_session_t *zs, _z_keyexpr_t keyexpr);

#if Z_FEATURE_PUBLICATION == 1
/**
 * Declare a :c:type:`_z_publisher_t` for the given resource key.
 *
 * Written resources that match the given key will only be sent on the network
 * if matching subscribers exist in the system.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     keyexpr:  The resource key to publish. The callee gets the ownership
 *              of any allocated value.
 *     encoding: The optional default encoding to use during put. The callee gets the ownership.
 *     reliability: The reliability of the publisher messages
 *
 * Returns:
 *    The created :c:type:`_z_publisher_t` (in null state if the declaration failed)..
 */
_z_publisher_t _z_declare_publisher(const _z_session_rc_t *zn, _z_keyexpr_t keyexpr, _z_encoding_t *encoding,
                                    z_congestion_control_t congestion_control, z_priority_t priority, bool is_express,
                                    z_reliability_t reliability);

/**
 * Undeclare a :c:type:`_z_publisher_t`.
 *
 * Parameters:
 *     pub: The :c:type:`_z_publisher_t` to undeclare. The callee releases the
 *          publisher upon successful return.
 * Returns:
 *    0 if success, or a negative value identifying the error.
 */
z_result_t _z_undeclare_publisher(_z_publisher_t *pub);

/**
 * Write data corresponding to a given resource key, allowing the definition of
 * additional properties.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     keyexpr: The resource key to write. The caller keeps its ownership.
 *     payload: The data to write.
 *     encoding: The encoding of the payload. The callee gets the ownership of
 *               any allocated value.
 *     kind: The kind of the value.
 *     cong_ctrl: The congestion control of this write. Possible values defined
 *                in :c:type:`_z_congestion_control_t`.
 *     is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *     timestamp: The timestamp of this write. The API level timestamp (e.g. of the data when it was created).
 *     attachment: An optional attachment to this write.
 *     reliability: The message reliability.
 *     source_info: The message source info.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
z_result_t _z_write(_z_session_t *zn, const _z_keyexpr_t keyexpr, _z_bytes_t payload, const _z_encoding_t *encoding,
                    const z_sample_kind_t kind, const z_congestion_control_t cong_ctrl, z_priority_t priority,
                    bool is_express, const _z_timestamp_t *timestamp, const _z_bytes_t attachment,
                    z_reliability_t reliability, const _z_source_info_t *source_info);
#endif

#if Z_FEATURE_SUBSCRIPTION == 1
/**
 * Declare a :c:type:`_z_subscriber_t` for the given resource key.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     keyexpr: The resource key to subscribe. The callee gets the ownership
 *             of any allocated value.
 *     callback: The callback function that will be called each time a data matching the subscribed resource is
 * received. arg: A pointer that will be passed to the **callback** on each call.
 *
 * Returns:
 *    The created :c:type:`_z_subscriber_t` (in null state if the declaration failed).
 */
_z_subscriber_t _z_declare_subscriber(const _z_session_rc_t *zn, _z_keyexpr_t keyexpr,
                                      _z_closure_sample_callback_t callback, _z_drop_handler_t dropper, void *arg);

/**
 * Undeclare a :c:type:`_z_subscriber_t`.
 *
 * Parameters:
 *     sub: The :c:type:`_z_subscriber_t` to undeclare. The callee releases the
 *          subscriber upon successful return.
 * Returns:
 *    0 if success, or a negative value identifying the error.
 */
z_result_t _z_undeclare_subscriber(_z_subscriber_t *sub);
#endif

#if Z_FEATURE_QUERYABLE == 1
/**
 * Declare a :c:type:`_z_queryable_t` for the given resource key.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     keyexpr: The resource key the :c:type:`_z_queryable_t` will reply to.
 *             The callee gets the ownership of any allocated value.
 *     complete: The complete of :c:type:`_z_queryable_t`.
 *     callback: The callback function that will be called each time a matching query is received.
 *     arg: A pointer that will be passed to the **callback** on each call.
 *
 * Returns:
 *    The created :c:type:`_z_queryable_t` (in null state if the declaration failed)..
 */
_z_queryable_t _z_declare_queryable(const _z_session_rc_t *zn, _z_keyexpr_t keyexpr, bool complete,
                                    _z_closure_query_callback_t callback, _z_drop_handler_t dropper, void *arg);

/**
 * Undeclare a :c:type:`_z_queryable_t`.
 *
 * Parameters:
 *     qle: The :c:type:`_z_queryable_t` to undeclare. The callee releases the
 *          queryable upon successful return.
 * Returns:
 *    0 if success, or a negative value identifying the error.
 */
z_result_t _z_undeclare_queryable(_z_queryable_t *qle);

/**
 * Send a reply to a query.
 *
 * This function must be called inside of a Queryable callback passing the
 * query received as parameters of the callback function. This function can
 * be called multiple times to send multiple replies to a query. The reply
 * will be considered complete when the Queryable callback returns.
 *
 * Parameters:
 *     query: The query to reply to. The caller keeps its ownership.
 *     key: The resource key of this reply. The caller keeps the ownership.
 *     payload: The value of this reply, the caller keeps ownership.
 *     kind: The type of operation.
 *     attachment: An optional attachment to the reply.
 */
z_result_t _z_send_reply(const _z_query_t *query, const _z_session_rc_t *zsrc, const _z_keyexpr_t *keyexpr,
                         const _z_value_t payload, const z_sample_kind_t kind, const z_congestion_control_t cong_ctrl,
                         z_priority_t priority, bool is_express, const _z_timestamp_t *timestamp,
                         const _z_bytes_t attachment);
/**
 * Send a reply error to a query.
 *
 * This function must be called inside of a Queryable callback passing the
 * query received as parameters of the callback function. This function can
 * be called multiple times to send multiple replies to a query. The reply
 * will be considered complete when the Queryable callback returns.
 *
 * Parameters:
 *     query: The query to reply to. The caller keeps its ownership.
 *     key: The resource key of this reply. The caller keeps the ownership.
 *     payload: The value of this reply, the caller keeps ownership.
 */
z_result_t _z_send_reply_err(const _z_query_t *query, const _z_session_rc_t *zsrc, const _z_value_t payload);
#endif

#if Z_FEATURE_QUERY == 1
/**
 * Declare a :c:type:`_z_querier_t` for the given resource key.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     keyexpr: The resource key to query. The callee gets the ownership of any
 *              allocated value.
 *     consolidation_mode: The kind of consolidation that should be applied on replies.
 *     congestion_control: The congestion control to apply when routing the querier queries.
 *     target: The kind of queryables that should be target of this query.
 *     priority: The priority of the query.
 *     is_express: If true, Zenoh will not wait to batch this operation with others to reduce the bandwidth.
 *     timeout_ms: The timeout value of this query.
 * Returns:
 *    The created :c:type:`_z_querier_t` (in null state if the declaration failed)..
 */
_z_querier_t _z_declare_querier(const _z_session_rc_t *zn, _z_keyexpr_t keyexpr,
                                z_consolidation_mode_t consolidation_mode, z_congestion_control_t congestion_control,
                                z_query_target_t target, z_priority_t priority, bool is_express, uint64_t timeout_ms,
                                _z_encoding_t *encoding, z_reliability_t reliability);

/**
 * Undeclare a :c:type:`_z_querier_t`.
 *
 * Parameters:
 *     querier: The :c:type:`_z_querier_t` to undeclare. The callee releases the
 *          querier upon successful return.
 * Returns:
 *    0 if success, or a negative value identifying the error.
 */
z_result_t _z_undeclare_querier(_z_querier_t *querier);

/**
 * Query data from the matching queryables in the system.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     keyexpr: The resource key to query. The callee gets the ownership of any
 *              allocated value.
 *     parameters: An indication to matching queryables about the queried data.
 *     target: The kind of queryables that should be target of this query.
 *     consolidation: The kind of consolidation that should be applied on replies.
 *     value: The payload of the query.
 *     callback: The callback function that will be called on reception of replies for this query.
 *     dropper: The callback function that will be called on upon completion of the callback.
 *     arg: A pointer that will be passed to the **callback** on each call.
 *     timeout_ms: The timeout value of this query.
 *     attachment: An optional attachment to this query.
 *     cong_ctrl: The congestion control to apply when routing the query.
 *     priority: The priority of the query.
 *
 */
z_result_t _z_query(_z_session_t *zn, _z_keyexpr_t keyexpr, const char *parameters, const z_query_target_t target,
                    const z_consolidation_mode_t consolidation, const _z_value_t value,
                    _z_closure_reply_callback_t callback, _z_drop_handler_t dropper, void *arg, uint64_t timeout_ms,
                    const _z_bytes_t attachment, z_congestion_control_t cong_ctrl, z_priority_t priority,
                    bool is_express);
#endif

#if Z_FEATURE_INTEREST == 1
uint32_t _z_add_interest(_z_session_t *zn, _z_keyexpr_t keyexpr, _z_interest_handler_t callback, uint8_t flags,
                         void *arg);
z_result_t _z_remove_interest(_z_session_t *zn, uint32_t interest_id);
#endif

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_NET_PRIMITIVES_H */
