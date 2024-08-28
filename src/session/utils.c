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
#include "zenoh-pico/session/interest.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ clone helpers ------------------*/
_z_timestamp_t _z_timestamp_duplicate(const _z_timestamp_t *tstamp) {
    _z_timestamp_t ts;
    ts.id = tstamp->id;
    ts.time = tstamp->time;
    return ts;
}

void _z_timestamp_clear(_z_timestamp_t *tstamp) {
    memset(&tstamp->id, 0, sizeof(_z_id_t));
    tstamp->time = 0;
}

_Bool _z_timestamp_check(const _z_timestamp_t *stamp) { return _z_id_check(stamp->id); }

int8_t _z_session_generate_zid(_z_id_t *bs, uint8_t size) {
    int8_t ret = _Z_RES_OK;
    z_random_fill((uint8_t *)bs->id, size);
    return ret;
}

/*------------------ Init/Free/Close session ------------------*/
int8_t _z_session_init(_z_session_rc_t *zsrc, _z_id_t *zid) {
    int8_t ret = _Z_RES_OK;
    _z_session_t *zn = _Z_RC_IN_VAL(zsrc);

    // Initialize the counters to 1
    zn->_entity_id = 1;
    zn->_resource_id = 1;
    zn->_query_id = 1;

    // Initialize the data structs
    zn->_local_resources = NULL;
    zn->_remote_resources = NULL;
#if Z_FEATURE_SUBSCRIPTION == 1
    zn->_local_subscriptions = NULL;
    zn->_remote_subscriptions = NULL;
#endif
#if Z_FEATURE_QUERYABLE == 1
    zn->_local_queryable = NULL;
#endif
#if Z_FEATURE_QUERY == 1
    zn->_pending_queries = NULL;
#endif

#if Z_FEATURE_MULTI_THREAD == 1
    ret = _z_mutex_init(&zn->_mutex_inner);
    if (ret != _Z_RES_OK) {
        _z_transport_clear(&zn->_tp);
        return ret;
    }
#endif  // Z_FEATURE_MULTI_THREAD == 1

    zn->_local_zid = *zid;
    // Note session in transport
    switch (zn->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            zn->_tp._transport._unicast._session = zsrc;
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            zn->_tp._transport._multicast._session = zsrc;
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            zn->_tp._transport._raweth._session = zsrc;
            break;
        default:
            break;
    }
    return ret;
}

void _z_session_clear(_z_session_t *zn) {
#if Z_FEATURE_MULTI_THREAD == 1
    _zp_stop_read_task(zn);
    _zp_stop_lease_task(zn);
#endif
    _z_close(zn);
    // Clear Zenoh PID
    // Clean up transports
    _z_transport_clear(&zn->_tp);

    // Clean up the entities
    _z_flush_resources(zn);
#if Z_FEATURE_SUBSCRIPTION == 1
    _z_flush_subscriptions(zn);
#endif
#if Z_FEATURE_QUERYABLE == 1
    _z_flush_session_queryable(zn);
#endif
#if Z_FEATURE_QUERY == 1
    _z_flush_pending_queries(zn);
#endif
    _z_flush_interest(zn);

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_drop(&zn->_mutex_inner);
#endif  // Z_FEATURE_MULTI_THREAD == 1
}

int8_t _z_session_close(_z_session_t *zn, uint8_t reason) {
    int8_t ret = _Z_ERR_GENERIC;

    if (zn != NULL) {
        ret = _z_transport_close(&zn->_tp, reason);
    }

    return ret;
}

#if Z_FEATURE_MULTI_THREAD == 1
void _zp_session_lock_mutex(_z_session_t *zn) { (void)_z_mutex_lock(&zn->_mutex_inner); }
void _zp_session_unlock_mutex(_z_session_t *zn) { (void)_z_mutex_unlock(&zn->_mutex_inner); }
#else
void _zp_session_lock_mutex(_z_session_t *zn) { _ZP_UNUSED(zn); }
void _zp_session_unlock_mutex(_z_session_t *zn) { _ZP_UNUSED(zn); }
#endif
