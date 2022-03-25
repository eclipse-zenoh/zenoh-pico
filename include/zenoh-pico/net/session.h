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

#ifndef ZENOH_PICO_SESSION_NETAPI_H
#define ZENOH_PICO_SESSION_NETAPI_H

#include "zenoh-pico/session/session.h"
#include "zenoh-pico/utils/properties.h"

/**
 * A zenoh-net session.
 */
typedef struct
{
    _z_mutex_t mutex_inner;

    // Session counters // FIXME: move to transport check
    _z_zint_t resource_id;
    _z_zint_t entity_id;
    _z_zint_t pull_id;
    _z_zint_t query_id;

    // Session declarations
    _z_resource_list_t *local_resources;
    _z_resource_list_t *remote_resources;

    // Session subscriptions
    _z_subscriber_list_t *local_subscriptions;
    _z_subscriber_list_t *remote_subscriptions;

    // Session queryables
    _z_queryable_list_t *local_queryables;
    _z_pending_query_list_t *pending_queries;

    // Session transport.
    // Zenoh-pico is considering a single transport per session.
    _z_transport_t *tp;
    _z_transport_manager_t *tp_manager;
} _z_session_t;

/**
 * Open a zenoh-net session
 *
 * Parameters:
 *     config: A set of properties. The caller keeps its ownership.
 *
 * Returns:
 *     A pointer of A :c:type:`_z_session_t` containing the created zenoh-net
 *     session or null if the creation did not succeed.
 */
_z_session_t *_z_open(_z_properties_t *config);

/**
 * Close a zenoh-net session.
 *
 * Parameters:
 *     session: A zenoh-net session. The callee releases session upon successful return.
 */
void _z_close(_z_session_t *session);

/**
 * Get informations about an zenoh-net session.
 *
 * Parameters:
 *     session: A zenoh-net session. The caller keeps its ownership.
 *
 * Returns:
 *     A :c:type:`_z_properties_t` map containing informations on the given zenoh-net session.
 */
_z_properties_t *_z_info(const _z_session_t *session);


/*------------------ Zenoh-Pico Session Management Auxiliar------------------*/

/**
 * Read from the network. This function should be called manually called when
 * the read loop has not been started, e.g., when running in a single thread.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int _zp_read(_z_session_t *z);

/**
 * Send a KeepAlive message.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int _zp_send_keep_alive(_z_session_t *z);

/**
 * Start a separate task to read from the network and process the messages
 * as soon as they are received. Note that the task can be implemented in
 * form of thread, process, etc. and its implementation is platform-dependent.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int _zp_start_read_task(_z_session_t *z);

/**
 * Stop the read task. This may result in stopping a thread or a process depending
 * on the target platform.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int _zp_stop_read_task(_z_session_t *z);

/**
 * Start a separate task to handle the session lease. This task will send ``KeepAlive``
 * messages when needed and will close the session when the lease is expired. Note that
 * the task can be implemented in form of thread, process, etc. and its implementation
 * is platform-dependent.
 *
 * In case of a multicast transport, this task will also send periodic ``Join``
 * messages.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int _zp_start_lease_task(_z_session_t *z);

/**
 * Stop the lease task. This may result in stopping a thread or a process depending
 * on the target platform.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int _zp_stop_lease_task(_z_session_t *z);

#endif /* ZENOH_PICO_SESSION_NETAPI_H */
