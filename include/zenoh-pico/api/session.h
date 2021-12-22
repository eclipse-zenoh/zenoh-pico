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

#ifndef ZENOH_PICO_SESSION_API_H
#define ZENOH_PICO_SESSION_API_H

#include "zenoh-pico/session/session.h"
#include "zenoh-pico/utils/properties.h"

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


/*------------------ Zenoh-Pico Session Management Auxiliar------------------*/

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

#endif /* ZENOH_PICO_SESSION_API_H */
