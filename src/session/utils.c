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
    ret = _z_mutex_init(&zn->_mutex_inner);
    if (ret != _Z_RES_OK) {
        return ret;
    }
#endif
    zn->_mode = Z_WHATAMI_CLIENT;
    // Initialize the counters to 1
    zn->_entity_id = 1;
    zn->_resource_id = 1;
    zn->_query_id = 1;

#if Z_FEATURE_AUTO_RECONNECT == 1
    _z_config_init(&zn->_config);
    zn->_decalaration_cache = NULL;
#endif

    // Initialize the data structs
    zn->_local_resources = NULL;
    zn->_remote_resources = NULL;
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

#if Z_FEATURE_MATCHING == 1
    zn->_matching_listeners = _z_matching_listener_intmap_make();
#endif

    _z_interest_init(zn);

    zn->_local_zid = *zid;

    return ret;
}

void _z_session_clear(_z_session_t *zn) {
    if (_z_session_is_closed(zn)) {
        return;
    }
#if Z_FEATURE_MULTI_THREAD == 1
    _zp_stop_read_task(zn);
    _zp_stop_lease_task(zn);
#endif

#if Z_FEATURE_AUTO_RECONNECT == 1
    _z_config_clear(&zn->_config);
    _z_network_message_list_free(&zn->_decalaration_cache);
#endif

    _z_close(zn);
    // Clear Zenoh PID
    // Clean up transports
    _z_transport_clear(&zn->_tp);

    // Clean up the entities
    _z_flush_resources(zn);
#if Z_FEATURE_SUBSCRIPTION == 1
    _z_flush_subscriptions(zn);
#if Z_FEATURE_RX_CACHE == 1
    _z_subscription_lru_cache_delete(&zn->_subscription_cache);
#endif
#endif
#if Z_FEATURE_QUERYABLE == 1
    _z_flush_session_queryable(zn);
#if Z_FEATURE_RX_CACHE == 1
    _z_queryable_lru_cache_delete(&zn->_queryable_cache);
#endif
#endif
#if Z_FEATURE_QUERY == 1
    _z_flush_pending_queries(zn);
#endif
#if Z_FEATURE_LIVELINESS == 1
    _z_liveliness_clear(zn);
#endif

#if Z_FEATURE_MATCHING == 1
    _z_matching_listener_intmap_clear(&zn->_matching_listeners);
#endif

    _z_flush_interest(zn);

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_drop(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1
}

z_result_t _z_session_close(_z_session_t *zn, uint8_t reason) {
    z_result_t ret = _Z_ERR_GENERIC;

    if (zn != NULL) {
        ret = _z_transport_close(&zn->_tp, reason);
    }

    return ret;
}
