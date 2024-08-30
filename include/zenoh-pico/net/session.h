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

#ifndef INCLUDE_ZENOH_PICO_NET_SESSION_H
#define INCLUDE_ZENOH_PICO_NET_SESSION_H

#include <stdint.h>

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/utils/config.h"

/**
 * A zenoh-net session.
 */
typedef struct _z_session_t {
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_t _mutex_inner;
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Zenoh-pico is considering a single transport per session.
    _z_transport_t _tp;

    // Zenoh PID
    _z_id_t _local_zid;

    // Session counters
    uint16_t _resource_id;
    uint32_t _entity_id;
    _z_zint_t _query_id;
    _z_zint_t _interest_id;

    // Session declarations
    _z_resource_list_t *_local_resources;
    _z_resource_list_t *_remote_resources;

    // Session subscriptions
#if Z_FEATURE_SUBSCRIPTION == 1
    _z_subscription_rc_list_t *_local_subscriptions;
    _z_subscription_rc_list_t *_remote_subscriptions;
#endif

    // Session queryables
#if Z_FEATURE_QUERYABLE == 1
    _z_session_queryable_rc_list_t *_local_queryable;
#endif
#if Z_FEATURE_QUERY == 1
    _z_pending_query_list_t *_pending_queries;
#endif

    // Session interests
#if Z_FEATURE_INTEREST == 1
    _z_session_interest_rc_list_t *_local_interests;
    _z_declare_data_list_t *_remote_declares;
#endif
} _z_session_t;

extern void _z_session_clear(_z_session_t *zn);  // Forward declaration to avoid cyclical include

_Z_REFCOUNT_DEFINE(_z_session, _z_session)

/**
 * Open a zenoh-net session
 *
 * Parameters:
 *     config: A set of properties. The caller keeps its ownership.
 *     zn: A pointer of A :c:type:`_z_session_t` used as a return value.
 *
 * Returns:
 *     ``0`` in case of success, or a ``negative value`` in case of failure.
 *
 */
int8_t _z_open(_z_session_rc_t *zn, _z_config_t *config);

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
 *     A :c:type:`_z_config_t` map containing informations on the given zenoh-net session.
 */
_z_config_t *_z_info(const _z_session_t *session);

/*------------------ Zenoh-Pico Session Management Auxiliary ------------------*/
/**
 * Read from the network. This function should be called manually called when
 * the read loop has not been started, e.g., when running in a single thread.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int8_t _zp_read(_z_session_t *z);

/**
 * Send a KeepAlive message.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int8_t _zp_send_keep_alive(_z_session_t *z);

/**
 * Send a Join message.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int8_t _zp_send_join(_z_session_t *z);

#if Z_FEATURE_MULTI_THREAD == 1
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
int8_t _zp_start_read_task(_z_session_t *z, z_task_attr_t *attr);

/**
 * Stop the read task. This may result in stopping a thread or a process depending
 * on the target platform.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int8_t _zp_stop_read_task(_z_session_t *z);

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
int8_t _zp_start_lease_task(_z_session_t *z, z_task_attr_t *attr);

/**
 * Stop the lease task. This may result in stopping a thread or a process depending
 * on the target platform.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
int8_t _zp_stop_lease_task(_z_session_t *z);
#endif  // Z_FEATURE_MULTI_THREAD == 1

#endif /* INCLUDE_ZENOH_PICO_NET_SESSION_H */
