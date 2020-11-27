/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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

#ifndef ZENOH_NET_PICO_SESSION_H
#define ZENOH_NET_PICO_SESSION_H

#include <stdint.h>
#include "zenoh-pico/net/types.h"

/*------------------ Init/Config ------------------*/
/**
 * Initialise the zenoh runtime logger
 */
void z_init_logger(void);

/**
 * Create an empty set of properties for zenoh-net session configuration.
 */
zn_properties_t *zn_config_empty(void);

/**
 * Create a default set of properties for client mode zenoh-net session configuration.
 * If peer is not null, it is added to the configuration as remote peer.
 *
 * Parameters:
 *   peer: An optional peer locator.
 */
zn_properties_t *zn_config_client(const char *locator);

/**
 * Create a default set of properties for zenoh-net session configuration.
 */
zn_properties_t *zn_config_default(void);

/**
 * Create a default subscription info.
 */
zn_subinfo_t zn_subinfo_default(void);

/**
 * Create a default :c:type:`zn_target_t`.
 */
zn_target_t zn_target_default(void);

/*------------------ Scout/Open/Close ------------------*/
/**
 * Scout for routers and/or peers.
 *
 * Parameters:
 *     what: A whatami bitmask of zenoh entities kind to scout for.
 *     config: A set of properties to configure the scouting.
 *     scout_period: The time that should be spent scouting before returnng the results.
 *
 * Returns:
 *     An array of :c:struct:`zn_hello_t` messages.
 */
zn_hello_array_t zn_scout(unsigned int what, zn_properties_t *config, unsigned long scout_period);

/**
 * Free an array of :c:struct:`zn_hello_t` messages and it's contained :c:struct:`zn_hello_t` messages recursively.
 *
 * Parameters:
 *     strs: The array of :c:struct:`zn_hello_t` messages to free.
 */
void zn_hello_array_free(zn_hello_array_t hellos);

/**
 * Open a zenoh-net session
 *
 * Parameters:
 *     config: A set of properties.
 *
 * Returns:
 *     The created zenoh-net session or null if the creation did not succeed.
 */
zn_session_t *zn_open(zn_properties_t *config);

/**
 * Close a zenoh-net session.
 *
 * Parameters:
 *     session: A zenoh-net session.
 */
void zn_close(zn_session_t *session);

/**
 * Get informations about an zenoh-net session.
 *
 * Parameters:
 *     session: A zenoh-net session.
 *
 * Returns:
 *     A :c:type:`zn_properties_t` map containing informations on the given zenoh-net session.
 */
zn_properties_t *zn_info(zn_session_t *session);

/*------------------ Declarations ------------------*/
/**
 * Associate a numerical id with the given resource key.
 *
 * This numerical id will be used on the network to save bandwidth and
 * ease the retrieval of the concerned resource in the routing tables.
 *
 * Parameters:
 *     session: The zenoh-net session.
 *     resource: The resource key to map to a numerical id.
 *
 * Returns:
 *     A numerical id.
 */
z_zint_t zn_declare_resource(zn_session_t *session, zn_reskey_t reskey);

/**
 * Associate a numerical id with the given resource key.
 *
 * This numerical id will be used on the network to save bandwidth and
 * ease the retrieval of the concerned resource in the routing tables.
 *
 * Parameters:
 *     session: The zenoh-net session.
 *     resource: The resource key to map to a numerical id.
 *
 * Returns:
 *     A numerical id.
 */
void zn_undeclare_resource(zn_session_t *session, z_zint_t rid);

/**
 * Declare a :c:type:`zn_publisher_t` for the given resource key.
 *
 * Written resources that match the given key will only be sent on the network
 * if matching subscribers exist in the system.
 *
 * Parameters:
 *     session: The zenoh-net session.
 *     resource: The resource key to publish.
 *
 * Returns:
 *    The created :c:type:`zn_publisher_t` or null if the declaration failed.
 */
zn_publisher_t *zn_declare_publisher(zn_session_t *session, zn_reskey_t reskey);

/**
 * Undeclare a :c:type:`zn_publisher_t`.
 *
 * Parameters:
 *     sub: The :c:type:`zn_publisher_t` to undeclare.
 */
void zn_undeclare_publisher(zn_publisher_t *publ);

/**
 * Declare a :c:type:`zn_subscriber_t` for the given resource key.
 *
 * Parameters:
 *     session: The zenoh-net session.
 *     resource: The resource key to subscribe.
 *     sub_info: The :c:type:`zn_subinfo_t` to configure the :c:type:`zn_subscriber_t`.
 *     callback: The callback function that will be called each time a data matching the subscribed resource is received.
 *     arg: A pointer that will be passed to the **callback** on each call.
 *
 * Returns:
 *    The created :c:type:`zn_subscriber_t` or null if the declaration failed.
 */
zn_subscriber_t *zn_declare_subscriber(zn_session_t *session,
                                       zn_reskey_t reskey,
                                       zn_subinfo_t sub_info,
                                       zn_data_handler_t callback,
                                       void *arg);

/**
 * Undeclare a :c:type:`zn_subscriber_t`.
 *
 * Parameters:
 *     sub: The :c:type:`zn_subscriber_t` to undeclare.
 */
void zn_undeclare_subscriber(zn_subscriber_t *sub);

/**
 * Declare a :c:type:`zn_queryable_t` for the given resource key.
 *
 * Parameters:
 *     session: The zenoh-net session.
 *     resource: The resource key the :c:type:`zn_queryable_t` will reply to.
 *     kind: The kind of :c:type:`zn_queryable_t`.
 *     callback: The callback function that will be called each time a matching query is received.
 *     arg: A pointer that will be passed to the **callback** on each call.
 *
 * Returns:
 *    The created :c:type:`zn_queryable_t` or null if the declaration failed.
 */
zn_queryable_t *zn_declare_queryable(zn_session_t *session,
                                     zn_reskey_t reskey,
                                     unsigned int kind,
                                     zn_queryable_handler_t callback,
                                     void *arg);

/**
 * Undeclare a :c:type:`zn_queryable_t`.
 *
 * Parameters:
 *     qle: The :c:type:`zn_queryable_t` to undeclare.
 */
void zn_undeclare_queryable(zn_queryable_t *qle);

/*------------------ Operations ------------------*/
/**
 * Create a resource key from a resource id.
 *
 * Parameters:
 *     rid: The resource id.
 *
 * Returns:
 *     Return a new resource key.
 */
zn_reskey_t zn_rid(unsigned long rid);

/**
 * Create a resource key from a resource name.
 *
 * Parameters:
 *     rname: The resource name.
 *
 * Returns:
 *     Return a new resource key.
 */
zn_reskey_t zn_rname(const char *rname);

/**
 * Create a resource key from a resource id and a suffix.
 *
 * Parameters:
 *     id: The resource id.
 *     suffix: The suffix.
 *
 * Returns:
 *     A new resource key.
 */
zn_reskey_t zn_rid_with_suffix(unsigned long id, const char *suffix);

/**
 * Free a :c:type:`zn_sample_t` contained key and value.
 *
 * Parameters:
 *     sample: The :c:type:`zn_sample_t` to free.
 */
void zn_sample_free(zn_sample_t sample);

/**
 * Write data.
 *
 * Parameters:
 *     session: The zenoh-net session.
 *     resource: The resource key to write.
 *     payload: The value to write.
 *     len: The length of the value to write.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int zn_write(zn_session_t *zn, zn_reskey_t reskey, const uint8_t *payload, size_t len);

/**
 * Write data.
 *
 * Parameters:
 *     session: The zenoh-net session.
 *     resource: The resource key to write.
 *     payload: The value to write.
 *     len: The length of the value to write.
 *     encoding: The encoding of the payload.
 *     kind: The kind of the value.
 *     cong_ctrl: The congestion control of this write.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int zn_write_ext(zn_session_t *zn, zn_reskey_t reskey, const uint8_t *payload, size_t len, uint8_t encoding, uint8_t kind, zn_congestion_control_t cong_ctrl);

/**
 * Pull data for a pull mode :c:type:`zn_subscriber_t`. The pulled data will be provided
 * by calling the **callback** function provided to the :c:func:`zn_declare_subscriber` function.
 *
 * Parameters:
 *     sub: The :c:type:`zn_subscriber_t` to pull from.
 */
int zn_pull(zn_subscriber_t *sub);

/**
 * Query data from the matching queryables in the system.
 *
 * Parameters:
 *     session: The zenoh-net session.
 *     resource: The resource key to query.
 *     predicate: An indication to matching queryables about the queried data.
 *     target: The kind of queryables that should be target of this query.
 *     consolidation: The kind of consolidation that should be applied on replies.
 *     callback: The callback function that will be called on reception of replies for this query.
 *     arg: A pointer that will be passed to the **callback** on each call.
 */
void zn_query(zn_session_t *session,
              zn_reskey_t reskey,
              const char *predicate,
              zn_query_target_t target,
              zn_query_consolidation_t consolidation,
              zn_query_handler_t callback,
              void *arg);

/**
 * Query data from the matching queryables in the system.
 * Replies are collected in an array.
 *
 * Parameters:
 *     session: The zenoh-net session.
 *     resource: The resource key to query.
 *     predicate: An indication to matching queryables about the queried data.
 *     target: The kind of queryables that should be target of this query.
 *     consolidation: The kind of consolidation that should be applied on replies.
 *
 * Returns:
 *    An array containing all the replies for this query.
 */
zn_reply_data_array_t zn_query_collect(zn_session_t *session,
                                       zn_reskey_t reskey,
                                       const char *predicate,
                                       zn_query_target_t target,
                                       zn_query_consolidation_t consolidation);

/**
 * Free a :c:type:`zn_reply_data_array_t` and it's contained replies.
 *
 * Parameters:
 *     replies: The :c:type:`zn_reply_data_array_t` to free.
 */
void zn_reply_data_array_free(zn_reply_data_array_t replies);

/**
 * Create a default :c:type:`zn_query_consolidation_t`.
 */
zn_query_consolidation_t zn_query_consolidation_default(void);

/**
 * Get the predicate of a received query.
 *
 * Parameters:
 *     query: The query.
 *
 * Returns:
 *     The predicate of the query.
 */
z_string_t zn_query_predicate(zn_query_t *query);

/**
 * Get the resource name of a received query.
 *
 * Parameters:
 *     query: The query.
 *
 * Returns:
 *     The resource name of the query.
 */
z_string_t zn_query_res_name(zn_query_t *query);

/**
 * Create a default :c:type:`zn_query_target_t`.
 */
zn_query_target_t zn_query_target_default(void);

/**
 * Send a reply to a query.
 *
 * This function must be called inside of a Queryable callback passing the
 * query received as parameters of the callback function. This function can
 * be called multiple times to send multiple replies to a query. The reply
 * will be considered complete when the Queryable callback returns.
 *
 * Parameters:
 *     query: The query to reply to.
 *     key: The resource key of this reply.
 *     payload: The value of this reply.
 *     len: The length of the value of this reply.
 */
void zn_send_reply(zn_query_t *query, const char *key, const uint8_t *payload, size_t len);

/*------------------ Zenoh-pico operations ------------------*/
/**
 * Read from the network. This function should be called manually called when 
 * the read loop has not been started, e.g., when running in a single thread.
 *
 * Parameters:
 *     session: The zenoh-net session.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int znp_read(zn_session_t *z);

/**
 * Send a KeepAlive message.
 *
 * Parameters:
 *     session: The zenoh-net session.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int znp_send_keep_alive(zn_session_t *z);

/**
 * Start a separate task to read from the network and process the messages
 * as soon as they are received. Note that the task can be implemented in 
 * form of thread, process, etc. and its implementation is platform-dependent.
 *
 * Parameters:
 *     session: The zenoh-net session.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int znp_start_read_task(zn_session_t *z);

/**
 * Stop the read task. This may result in stopping a thread or a process depending
 * on the target platform.
 *
 * Parameters:
 *     session: The zenoh-net session.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int znp_stop_read_task(zn_session_t *z);

/**
 * Start a separate task to handle the session lease. This task will send ``KeepAlive``
 * messages when needed and will close the session when the lease is expired. Note that 
 * the task can be implemented in form of thread, process, etc. and its implementation 
 * is platform-dependent.
 *
 * Parameters:
 *     session: The zenoh-net session.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int znp_start_lease_task(zn_session_t *z);

/**
 * Stop the lease task. This may result in stopping a thread or a process depending
 * on the target platform.
 *
 * Parameters:
 *     session: The zenoh-net session.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int znp_stop_lease_task(zn_session_t *z);

#endif /* ZENOH_NET_PICO_SESSION_H */
