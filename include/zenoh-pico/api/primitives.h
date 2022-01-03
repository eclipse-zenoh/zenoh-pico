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

#ifndef ZENOH_PICO_PRIMITIVES_API_H
#define ZENOH_PICO_PRIMITIVES_API_H

#include "zenoh-pico/api/session.h"
#include "zenoh-pico/api/subscribe.h"
#include "zenoh-pico/api/publish.h"
#include "zenoh-pico/api/query.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/encoding.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/utils/properties.h"

/*------------------ Discovery ------------------*/

/**
 * Scout for routers and/or peers.
 *
 * Parameters:
 *     what: A whatami bitmask of zenoh entities kind to scout for.
 *     config: A set of properties to configure the scouting.
 *     scout_period: The time that should be spent scouting before returnng the results.
 *
 * Returns:
 *     An array of :c:type:`zn_hello_t` messages.
 *     The caller gets its ownership, thus must be released using :c:function:`zn_hello_array_free`.
 */
zn_hello_array_t zn_scout(const unsigned int what, const zn_properties_t *config, const unsigned long scout_period);


/*------------------ Declarations ------------------*/

/**
 * Associate a numerical id with the given resource key.
 *
 * This numerical id will be used on the network to save bandwidth and
 * ease the retrieval of the concerned resource in the routing tables.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     reskey: The resource key to map to a numerical id. The callee gets
 *             the ownership of any allocated value.
 *
 * Returns:
 *     A numerical id of the declared resource.
 */
z_zint_t zn_declare_resource(zn_session_t *zn, zn_reskey_t reskey);

/**
 * Associate a numerical id with the given resource key.
 *
 * This numerical id will be used on the network to save bandwidth and
 * ease the retrieval of the concerned resource in the routing tables.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     rid: The numerical id of the resource to undeclare.
 */
void zn_undeclare_resource(zn_session_t *zn, const z_zint_t rid);

/**
 * Declare a :c:type:`zn_publisher_t` for the given resource key.
 *
 * Written resources that match the given key will only be sent on the network
 * if matching subscribers exist in the system.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     reskey:  The resource key to publish. The callee gets the ownership
 *              of any allocated value.
 *
 * Returns:
 *    The created :c:type:`zn_publisher_t` or null if the declaration failed.
 */
zn_publisher_t *zn_declare_publisher(zn_session_t *zn, zn_reskey_t reskey);

/**
 * Undeclare a :c:type:`zn_publisher_t`.
 *
 * Parameters:
 *     pub: The :c:type:`zn_publisher_t` to undeclare. The callee releases the
 *          publisher upon successful return.
 */
void zn_undeclare_publisher(zn_publisher_t *pub);

/**
 * Declare a :c:type:`zn_subscriber_t` for the given resource key.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     reskey: The resource key to subscribe. The callee gets the ownership
 *             of any allocated value.
 *     sub_info: The :c:type:`zn_subinfo_t` to configure the :c:type:`zn_subscriber_t`.
 *               The callee gets the ownership of any allocated value.
 *     callback: The callback function that will be called each time a data matching the subscribed resource is received.
 *     arg: A pointer that will be passed to the **callback** on each call.
 *
 * Returns:
 *    The created :c:type:`zn_subscriber_t` or null if the declaration failed.
 */
zn_subscriber_t *zn_declare_subscriber(zn_session_t *zn,
                                       zn_reskey_t reskey,
                                       zn_subinfo_t sub_info,
                                       zn_data_handler_t callback,
                                       void *arg);

/**
 * Undeclare a :c:type:`zn_subscriber_t`.
 *
 * Parameters:
 *     sub: The :c:type:`zn_subscriber_t` to undeclare. The callee releases the
 *          subscriber upon successful return.
 */
void zn_undeclare_subscriber(zn_subscriber_t *sub);

/**
 * Declare a :c:type:`zn_queryable_t` for the given resource key.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     reskey: The resource key the :c:type:`zn_queryable_t` will reply to.
 *             The callee gets the ownership of any allocated value.
 *     kind: The kind of :c:type:`zn_queryable_t`.
 *     callback: The callback function that will be called each time a matching query is received.
 *     arg: A pointer that will be passed to the **callback** on each call.
 *
 * Returns:
 *    The created :c:type:`zn_queryable_t` or null if the declaration failed.
 */
zn_queryable_t *zn_declare_queryable(zn_session_t *zn,
                                     zn_reskey_t reskey,
                                     unsigned int kind,
                                     zn_queryable_handler_t callback,
                                     void *arg);

/**
 * Undeclare a :c:type:`zn_queryable_t`.
 *
 * Parameters:
 *     qle: The :c:type:`zn_queryable_t` to undeclare. The callee releases the
 *          queryable upon successful return.
 */
void zn_undeclare_queryable(zn_queryable_t *qle);


/*------------------ Operations ------------------*/

/**
 * Write data corresponding to a given resource key.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     reskey: The resource key to write. The caller keeps its ownership.
 *     payload: The value to write.
 *     len: The length of the value to write.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int zn_write(zn_session_t *zn, const zn_reskey_t reskey, const uint8_t *payload, const size_t len);

/**
 * Write data corresponding to a given resource key, allowing the definition of
 * additional properties.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     reskey: The resource key to write. The caller keeps its ownership.
 *     payload: The value to write.
 *     len: The length of the value to write.
 *     encoding: The encoding of the payload. The callee gets the ownership of
 *               any allocated value.
 *     kind: The kind of the value.
 *     cong_ctrl: The congestion control of this write. Possible values defined
 *                in :c:type:`zn_congestion_control_t`.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int zn_write_ext(zn_session_t *zn, const zn_reskey_t reskey, const uint8_t *payload, const size_t len, uint8_t encoding, const uint8_t kind, const zn_congestion_control_t cong_ctrl);

/**
 * Pull data for a pull mode :c:type:`zn_subscriber_t`. The pulled data will be provided
 * by calling the **callback** function provided to the :c:func:`zn_declare_subscriber` function.
 *
 * Parameters:
 *     sub: The :c:type:`zn_subscriber_t` to pull from.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int zn_pull(const zn_subscriber_t *sub);

/**
 * Query data from the matching queryables in the system.
 *
 * Parameters:
 *     zn: The zenoh-net session. The caller keeps its ownership.
 *     reskey: The resource key to query. The callee gets the ownership of any
 *             allocated value.
 *     predicate: An indication to matching queryables about the queried data.
 *     target: The kind of queryables that should be target of this query.
 *     consolidation: The kind of consolidation that should be applied on replies.
 *     callback: The callback function that will be called on reception of replies for this query.
 *     arg: A pointer that will be passed to the **callback** on each call.
 */
void zn_query(zn_session_t *zn,
              zn_reskey_t reskey,
              const z_str_t predicate,
              const zn_query_target_t target,
              const zn_query_consolidation_t consolidation,
              zn_query_handler_t callback,
              void *arg);

/**
 * Query data from the matching queryables in the system.
 * Replies are collected in an array.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 *     reskey: The resource key to query. The callee gets the ownership of any
 *             allocated value.
 *     predicate: An indication to matching queryables about the queried data.
 *     target: The kind of queryables that should be target of this query.
 *     consolidation: The kind of consolidation that should be applied on replies.
 *
 * Returns:
 *    An array containing all the replies for this query.
 */
zn_reply_data_array_t zn_query_collect(zn_session_t *zn,
                                       zn_reskey_t reskey,
                                       const z_str_t predicate,
                                       const zn_query_target_t target,
                                       const zn_query_consolidation_t consolidation);

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
void zn_send_reply(zn_query_t *query, const z_str_t key, const uint8_t *payload, const size_t len);

#endif /* ZENOH_PICO_PRIMITIVES_API_H */
