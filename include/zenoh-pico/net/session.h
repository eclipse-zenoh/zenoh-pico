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
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/session/liveliness.h"
#include "zenoh-pico/session/matching.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/utils/config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A zenoh-net session.
 */
typedef struct _z_session_t {
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_t _mutex_inner;
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Zenoh-pico is considering a single transport per session.
    z_whatami_t _mode;
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

#if Z_FEATURE_AUTO_RECONNECT == 1
    // Information for session restoring
    _z_config_t _config;
    _z_network_message_list_t *_decalaration_cache;
    z_task_attr_t *_lease_task_attr;
    z_task_attr_t *_read_task_attr;
#endif

    // Session subscriptions
#if Z_FEATURE_SUBSCRIPTION == 1
    _z_subscription_rc_list_t *_subscriptions;
    _z_subscription_rc_list_t *_liveliness_subscriptions;
#if Z_FEATURE_RX_CACHE == 1
    _z_subscription_lru_cache_t _subscription_cache;
#endif
#endif

#if Z_FEATURE_LIVELINESS == 1
    _z_keyexpr_intmap_t _local_tokens;
    _z_keyexpr_intmap_t _remote_tokens;
#if Z_FEATURE_QUERY == 1
    uint32_t _liveliness_query_id;
    _z_liveliness_pending_query_intmap_t _liveliness_pending_queries;
#endif
#endif

    // Session queryables
#if Z_FEATURE_QUERYABLE == 1
    _z_session_queryable_rc_list_t *_local_queryable;
#if Z_FEATURE_RX_CACHE == 1
    _z_queryable_lru_cache_t _queryable_cache;
#endif
#endif
#if Z_FEATURE_QUERY == 1
    _z_pending_query_list_t *_pending_queries;
#endif

#if Z_FEATURE_MATCHING == 1
    _z_matching_listener_intmap_t _matching_listeners;
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
 *     zn: A pointer of A :c:type:`_z_session_rc_t` used as a return value.
 *     config: A set of properties. The caller keeps its ownership.
 *     zid: A pointer to Zenoh ID.
 *
 * Returns:
 *     ``0`` in case of success, or a ``negative value`` in case of failure.
 */
z_result_t _z_open(_z_session_rc_t *zn, _z_config_t *config, const _z_id_t *zid);

/**
 * Reopen a disconnected zenoh-net session
 *
 * Parameters:
 *     zn: Existing zenoh-net session.
 *
 * Returns:
 *     ``0`` in case of success, or a ``negative value`` in case of failure.
 */
z_result_t _z_reopen(_z_session_rc_t *zn);

/**
 * Store declaration network message to cache for resend it after session restore
 *
 * Parameters:
 *     zs: A zenoh-net session.
 *     z_msg: Network message with declaration
 */
void _z_cache_declaration(_z_session_t *zs, const _z_network_message_t *n_msg);

/**
 * Remove corresponding declaration from the cache
 *
 * Parameters:
 *     zs: A zenoh-net session.
 *     z_msg: Network message with undeclaration
 */
void _z_prune_declaration(_z_session_t *zs, const _z_network_message_t *n_msg);

/**
 * Close a zenoh-net session.
 *
 * Parameters:
 *     session: A zenoh-net session. The callee releases session upon successful return.
 */
void _z_close(_z_session_t *session);

/**
 * Return true is session and all associated transports were closed.
 */
bool _z_session_is_closed(const _z_session_t *session);

/**
 * Upgrades weak session session, than resets it to null if session is closed.
 */
_z_session_rc_t _z_session_weak_upgrade_if_open(const _z_session_weak_t *session);

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
z_result_t _zp_read(_z_session_t *z);

/**
 * Send a KeepAlive message.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
z_result_t _zp_send_keep_alive(_z_session_t *z);

/**
 * Send a Join message.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
z_result_t _zp_send_join(_z_session_t *z);

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
z_result_t _zp_start_read_task(_z_session_t *z, z_task_attr_t *attr);

/**
 * Stop the read task. This may result in stopping a thread or a process depending
 * on the target platform.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
z_result_t _zp_stop_read_task(_z_session_t *z);

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
z_result_t _zp_start_lease_task(_z_session_t *z, z_task_attr_t *attr);

/**
 * Stop the lease task. This may result in stopping a thread or a process depending
 * on the target platform.
 *
 * Parameters:
 *     session: The zenoh-net session. The caller keeps its ownership.
 * Returns:
 *     ``0`` in case of success, ``-1`` in case of failure.
 */
z_result_t _zp_stop_lease_task(_z_session_t *z);
#endif  // Z_FEATURE_MULTI_THREAD == 1

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_NET_SESSION_H */
