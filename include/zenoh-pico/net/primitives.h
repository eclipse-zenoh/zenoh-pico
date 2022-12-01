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

#ifndef ZENOH_PICO_PRIMITIVES_NETAPI_H
#define ZENOH_PICO_PRIMITIVES_NETAPI_H

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/net/publish.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/utils/config.h"

/*------------------ Discovery ------------------*/

/**
 * Scout for routers and/or peers.
 *
 * Parameters:
 *     what: A whatami bitmask of zenoh entities kind to scout for.
 *     locator: The locator where to scout.
 *     timeout: The time that should be spent scouting before returnng the results.
 */
void _z_scout(const z_whatami_t what, const char *locator, const uint32_t timeout, _z_hello_handler_t callback,
              void *arg_call, _z_drop_handler_t dropper, void *arg_drop);

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
_z_zint_t _z_declare_resource(_z_session_t *zn, _z_keyexpr_t keyexpr);

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
int8_t _z_undeclare_resource(_z_session_t *zn, const _z_zint_t rid);

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
 *
 * Returns:
 *    The created :c:type:`_z_publisher_t` or null if the declaration failed.
 */
_z_publisher_t *_z_declare_publisher(_z_session_t *zn, _z_keyexpr_t keyexpr, z_congestion_control_t congestion_control,
                                     z_priority_t priority);

/**
 * Undeclare a :c:type:`_z_publisher_t`.
 *
 * Parameters:
 *     pub: The :c:type:`_z_publisher_t` to undeclare. The callee releases the
 *          publisher upon successful return.
 * Returns:
 *    0 if success, or a negative value identifying the error.
 */
int8_t _z_undeclare_publisher(_z_publisher_t *pub);

/**
 * Declare a :c:type:`_z_subscriber_t` for the given resource key.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     keyexpr: The resource key to subscribe. The callee gets the ownership
 *             of any allocated value.
 *     sub_info: The :c:type:`_z_subinfo_t` to configure the :c:type:`_z_subscriber_t`.
 *               The callee gets the ownership of any allocated value.
 *     callback: The callback function that will be called each time a data matching the subscribed resource is
 * received. arg: A pointer that will be passed to the **callback** on each call.
 *
 * Returns:
 *    The created :c:type:`_z_subscriber_t` or null if the declaration failed.
 */
_z_subscriber_t *_z_declare_subscriber(_z_session_t *zn, _z_keyexpr_t keyexpr, _z_subinfo_t sub_info,
                                       _z_data_handler_t callback, _z_drop_handler_t dropper, void *arg);

/**
 * Undeclare a :c:type:`_z_subscriber_t`.
 *
 * Parameters:
 *     sub: The :c:type:`_z_subscriber_t` to undeclare. The callee releases the
 *          subscriber upon successful return.
 * Returns:
 *    0 if success, or a negative value identifying the error.
 */
int8_t _z_undeclare_subscriber(_z_subscriber_t *sub);

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
 *    The created :c:type:`_z_queryable_t` or null if the declaration failed.
 */
_z_queryable_t *_z_declare_queryable(_z_session_t *zn, _z_keyexpr_t keyexpr, _Bool complete,
                                     _z_questionable_handler_t callback, _z_drop_handler_t dropper, void *arg);

/**
 * Undeclare a :c:type:`_z_queryable_t`.
 *
 * Parameters:
 *     qle: The :c:type:`_z_queryable_t` to undeclare. The callee releases the
 *          queryable upon successful return.
 * Returns:
 *    0 if success, or a negative value identifying the error.
 */
int8_t _z_undeclare_queryable(_z_queryable_t *qle);

/*------------------ Operations ------------------*/

/**
 * Write data corresponding to a given resource key, allowing the definition of
 * additional properties.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     keyexpr: The resource key to write. The caller keeps its ownership.
 *     payload: The value to write.
 *     len: The length of the value to write.
 *     encoding: The encoding of the payload. The callee gets the ownership of
 *               any allocated value.
 *     kind: The kind of the value.
 *     cong_ctrl: The congestion control of this write. Possible values defined
 *                in :c:type:`_z_congestion_control_t`.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int8_t _z_write(_z_session_t *zn, const _z_keyexpr_t keyexpr, const uint8_t *payload, const size_t len,
                const _z_encoding_t encoding, const z_sample_kind_t kind, const z_congestion_control_t cong_ctrl);

/**
 * Pull data for a pull mode :c:type:`_z_subscriber_t`. The pulled data will be provided
 * by calling the **callback** function provided to the :c:func:`_z_declare_subscriber` function.
 *
 * Parameters:
 *     sub: The :c:type:`_z_subscriber_t` to pull from.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int8_t _z_subscriber_pull(const _z_subscriber_t *sub);

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
 *     with_value: The payload of the query.
 *     callback: The callback function that will be called on reception of replies for this query.
 *     arg_call: A pointer that will be passed to the **callback** on each call.
 *     dropper: The callback function that will be called on upon completion of the callback.
 *     arg_drop: A pointer that will be passed to the **dropper** on each call.
 */
int8_t _z_query(_z_session_t *zn, _z_keyexpr_t keyexpr, const char *parameters, const z_query_target_t target,
                const z_consolidation_mode_t consolidation, const _z_value_t with_value, _z_reply_handler_t callback,
                void *arg_call, _z_drop_handler_t dropper, void *arg_drop);

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
 *     payload: The value of this reply.
 *     len: The length of the value of this reply.
 */
int8_t _z_send_reply(const z_query_t *query, _z_keyexpr_t keyexpr, const uint8_t *payload, const size_t len);

#endif /* ZENOH_PICO_PRIMITIVES_NETAPI_H */
