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

#ifndef ZENOH_C_NET_SESSION_H
#define ZENOH_C_NET_SESSION_H

#include <stdint.h>
#include "zenoh/net/types.h"

/*------------------ Init/Config ------------------*/
/**
 * Initialise the zenoh runtime logger
 *
 */
void zn_init_logger(void);

/**
 * Create an empty set of properties for zenoh-net session configuration.
 * 
 */
zn_properties_t *zn_config_empty(void);

/**
 * Create a default set of properties for client mode zenoh-net session configuration.
 * If peer is not null, it is added to the configuration as remote peer.
 *
 * Parameters:
 *   peer: An optional peer locator.
 * 
 */
zn_properties_t *zn_config_client(const char *locator);

/**
 * Create a default set of properties for zenoh-net session configuration.
 * 
 */
zn_properties_t *zn_config_default(void);

/**
 * Create a default subscription info.
 * 
 */
zn_subinfo_t zn_subinfo_default(void);

/**
 * Create a default :c:type:`zn_target_t`.
 * 
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
 * 
 */
zn_hello_array_t zn_scout(unsigned int what, zn_properties_t *config, unsigned long scout_period);

/**
 * Free an array of :c:struct:`zn_hello_t` messages and it's contained :c:struct:`zn_hello_t` messages recursively.
 *
 * Parameters:
 *     strs: The array of :c:struct:`zn_hello_t` messages to free.
 *
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
 * 
 */
zn_session_t *zn_open(zn_properties_t *config);

/**
 * Close a zenoh-net session.
 *
 * Parameters:
 *     session: A zenoh-net session.
 * 
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
 * 
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
 * 
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
 * 
 */
int zn_undeclare_resource(zn_session_t *session, z_zint_t rid);

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
 * 
 */
zn_publisher_t *zn_declare_publisher(zn_session_t *session, zn_reskey_t reskey);

/**
 * Undeclare a :c:type:`zn_publisher_t`.
 *
 * Parameters:
 *     sub: The :c:type:`zn_publisher_t` to undeclare.
 * 
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
 * 
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
 * 
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
 * 
 */
zn_queryable_t *zn_declare_queryable(zn_session_t *session,
                                     zn_reskey_t reskey,
                                     unsigned int kind,
                                     zn_query_handler_t callback,
                                     void *arg);

/**
 * Undeclare a :c:type:`zn_queryable_t`.
 *
 * Parameters:
 *     qle: The :c:type:`zn_queryable_t` to undeclare.
 * 
 */
void zn_undeclare_queryable(zn_queryable_t *qle);

/*------------------ Operations ------------------*/
/**
 * Create a resource key from a resource id.
 *
 * Parameters:
 *     id: The resource id.
 *
 * Returns:
 *     Return a new resource key.
 * 
 */
zn_reskey_t zn_rid(const unsigned long rid);
zn_reskey_t zn_rname(const char *rname);

int zn_send_keep_alive(zn_session_t *z);

/**
 * Write data.
 *
 * Parameters:
 *     session: The zenoh-net session.
 *     resource: The resource key to write.
 *     payload: The value to write.
 *     len: The length of the value to write.
 * Returns:
 *     ``0`` in case of success, ``1`` in case of failure.
 */
int zn_write(zn_session_t *z, zn_reskey_t reskey, const uint8_t *payload, size_t len);

int zn_write_ext(zn_session_t *z, zn_reskey_t reskey, const uint8_t *payload, size_t len, uint8_t encoding, uint8_t kind, zn_congestion_control_t cong_ctrl);

int zn_read(zn_session_t *z);

int zn_pull(zn_subscriber_t *sub);

// int zn_query(zn_session_t *z, zn_reskey_t *resource, const char *predicate, zn_reply_handler_t reply_handler, void *arg);
// int zn_query_wo(zn_session_t *z, zn_reskey_t *resource, const char *predicate, zn_reply_handler_t reply_handler, void *arg, zn_query_dest_t dest_storages, zn_query_dest_t dest_evals);

#endif /* ZENOH_C_NET_SESSION_H */
