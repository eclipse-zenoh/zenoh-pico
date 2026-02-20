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
//

#include "zenoh-pico/session/utils.h"

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/session/liveliness.h"
#include "zenoh-pico/session/matching.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/utils/config.h"
#include "zenoh-pico/utils/result.h"

/*------------------ clone helpers ------------------*/
void _z_timestamp_copy(_z_timestamp_t *dst, const _z_timestamp_t *src) { *dst = *src; }

_z_timestamp_t _z_timestamp_duplicate(const _z_timestamp_t *tstamp) { return *tstamp; }

void _z_timestamp_move(_z_timestamp_t *dst, _z_timestamp_t *src) {
    *dst = *src;
    _z_timestamp_clear(src);
}

void _z_timestamp_clear(_z_timestamp_t *tstamp) {
    tstamp->valid = false;
    tstamp->time = 0;
}

z_result_t _z_session_generate_zid(_z_id_t *bs, uint8_t size) {
    z_result_t ret = _Z_RES_OK;
    z_random_fill((uint8_t *)bs->id, size);
    return ret;
}

/*------------------ Init/Free/Close session ------------------*/
z_result_t _z_session_init(_z_session_t *zn, const _z_id_t *zid) {
    z_result_t ret = _Z_RES_OK;

#if Z_FEATURE_MULTI_THREAD == 1
    zn->_mutex_inner_initialized = false;
    _Z_RETURN_IF_ERR(_z_mutex_init(&zn->_mutex_inner));
    zn->_mutex_inner_initialized = true;
#endif
    zn->_mode = Z_WHATAMI_CLIENT;
    zn->_tp._type = _Z_TRANSPORT_NONE;
#if Z_FEATURE_MULTI_THREAD == 1
    zn->_read_task_should_run = false;
    zn->_lease_task_should_run = false;
#endif
    // Initialize the counters to 1
    zn->_entity_id = 1;
    zn->_resource_id = 1;
    zn->_query_id = 1;

#if Z_FEATURE_AUTO_RECONNECT == 1
    _z_config_init(&zn->_config);
    zn->_declaration_cache = NULL;
#endif

    // Initialize the data structs
    zn->_local_resources = NULL;
#if Z_FEATURE_SUBSCRIPTION == 1
    zn->_subscriptions = NULL;
    zn->_liveliness_subscriptions = NULL;
#if Z_FEATURE_RX_CACHE == 1
    zn->_subscription_cache = _z_subscription_lru_cache_init(Z_RX_CACHE_SIZE);
#endif
#endif
#if Z_FEATURE_QUERYABLE == 1
    zn->_local_queryable = NULL;
#if Z_FEATURE_RX_CACHE == 1
    zn->_queryable_cache = _z_queryable_lru_cache_init(Z_RX_CACHE_SIZE);
#endif
#endif
#if Z_FEATURE_QUERY == 1
    zn->_pending_queries = NULL;
#endif

#if Z_FEATURE_LIVELINESS == 1
    _z_liveliness_init(zn);
#endif

#if Z_FEATURE_INTEREST == 1
    zn->_write_filters = NULL;
#endif

#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
#if Z_FEATURE_MULTI_THREAD == 1
    zn->_periodic_scheduler_task = NULL;
    zn->_periodic_task_should_run = false;
    zn->_periodic_scheduler_task_attr = NULL;
#endif
    ret = _zp_periodic_scheduler_init(&zn->_periodic_scheduler);
#endif

#if Z_FEATURE_ADMIN_SPACE == 1
    zn->_admin_space_queryable_id = 0;
#if Z_FEATURE_CONNECTIVITY == 1
    zn->_admin_space_transport_listener_id = 0;
    zn->_admin_space_link_listener_id = 0;
#endif
#endif

#if Z_FEATURE_CONNECTIVITY == 1
    zn->_connectivity_next_listener_id = 1;
    _z_connectivity_transport_listener_intmap_init(&zn->_connectivity_transport_event_listeners);
    _z_connectivity_link_listener_intmap_init(&zn->_connectivity_link_event_listeners);
#endif
#endif
    zn->_callback_drop_sync_group = _z_sync_group_null();
    _Z_SET_IF_OK(ret, _z_sync_group_create(&zn->_callback_drop_sync_group));
    if (ret != _Z_RES_OK) {
#if Z_FEATURE_MULTI_THREAD == 1
        zn->_mutex_inner_initialized = false;
        _z_mutex_drop(&zn->_mutex_inner);
#endif
        _z_sync_group_drop(&zn->_callback_drop_sync_group);
        _Z_ERROR_RETURN(ret);
    }

    _z_interest_init(zn);

    zn->_local_zid = *zid;
    return ret;
}

void _z_session_clear(_z_session_t *zn) {
    if (!_z_session_is_closed(zn)) {
#if Z_FEATURE_MULTI_THREAD == 1
        _zp_stop_read_task(zn);
        _zp_stop_lease_task(zn);
#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
        _zp_stop_periodic_scheduler_task(zn);
#endif
#endif
#endif

#if Z_FEATURE_AUTO_RECONNECT == 1
        _z_config_clear(&zn->_config);
        _z_network_message_slist_free(&zn->_declaration_cache);
#endif

        _z_close(zn);
        // Clear Zenoh PID
        // Clean up transports
        _z_transport_clear(&zn->_tp);

        // Clean up the entities
        _z_flush_local_resources(zn);
#if Z_FEATURE_SUBSCRIPTION == 1
        _z_flush_subscriptions(zn);
#endif
#if Z_FEATURE_QUERYABLE == 1
        // Admin space querable cleanup will occur as part of queryable cleanup
        _z_flush_session_queryable(zn);
#endif
#if Z_FEATURE_QUERY == 1
        _z_flush_pending_queries(zn);
#endif
#if Z_FEATURE_LIVELINESS == 1
        _z_liveliness_clear(zn);
#endif

        _z_flush_interest(zn);
    }

#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
    if (_zp_periodic_scheduler_check(&zn->_periodic_scheduler)) {
        _zp_periodic_scheduler_clear(&zn->_periodic_scheduler);
    }
#endif

#if Z_FEATURE_CONNECTIVITY == 1
    _z_connectivity_transport_listener_intmap_clear(&zn->_connectivity_transport_event_listeners);
    _z_connectivity_link_listener_intmap_clear(&zn->_connectivity_link_event_listeners);
    zn->_connectivity_next_listener_id = 1;
#if Z_FEATURE_ADMIN_SPACE == 1
    zn->_admin_space_transport_listener_id = 0;
    zn->_admin_space_link_listener_id = 0;
#endif
#endif
#endif

#if Z_FEATURE_MULTI_THREAD == 1
    zn->_read_task_should_run = false;
    zn->_lease_task_should_run = false;
#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
    zn->_periodic_task_should_run = false;
    zn->_periodic_scheduler_task_attr = NULL;
#endif
#endif  // Z_FEATURE_UNSTABLE_API
#endif  // Z_FEATURE_MULTI_THREAD == 1

#if Z_FEATURE_MULTI_THREAD == 1
    if (zn->_mutex_inner_initialized) {
        zn->_mutex_inner_initialized = false;
        _z_mutex_drop(&zn->_mutex_inner);
    }
#endif  // Z_FEATURE_MULTI_THREAD == 1
    _z_sync_group_wait(&zn->_callback_drop_sync_group);
    _z_sync_group_drop(&zn->_callback_drop_sync_group);
}

z_result_t _z_session_close(_z_session_t *zn, uint8_t reason) {
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    z_result_t ret = _Z_ERR_GENERIC;

    if (zn != NULL) {
        ret = _z_transport_close(&zn->_tp, reason);
    }

    return ret;
}
