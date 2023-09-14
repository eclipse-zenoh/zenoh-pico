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
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"

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

_Bool _z_timestamp_check(const _z_timestamp_t *stamp) {
    for (uint8_t i = 0; i < sizeof(_z_id_t); ++i)
        if (stamp->id.id[i]) return true;
    return false;
}

int8_t _z_session_generate_zid(_z_id_t *bs, uint8_t size) {
    int8_t ret = _Z_RES_OK;
    z_random_fill((uint8_t *)bs->id, size);
    return ret;
}

/*------------------ Init/Free/Close session ------------------*/
int8_t _z_session_init(_z_session_t *zn, _z_id_t *zid) {
    int8_t ret = _Z_RES_OK;

    // Initialize the counters to 1
    zn->_entity_id = 1;
    zn->_resource_id = 1;
    zn->_query_id = 1;
    zn->_pull_id = 1;

    // Initialize the data structs
    zn->_local_resources = NULL;
    zn->_remote_resources = NULL;
    zn->_local_subscriptions = NULL;
    zn->_remote_subscriptions = NULL;
    zn->_local_questionable = NULL;
    zn->_pending_queries = NULL;

#if Z_MULTI_THREAD == 1
    ret = _z_mutex_init(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1
    if (ret == _Z_RES_OK) {
        zn->_local_zid = *zid;
#if Z_UNICAST_TRANSPORT == 1
        if (zn->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
            zn->_tp._transport._unicast._session = zn;
        } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
            if (zn->_tp._type == _Z_TRANSPORT_MULTICAST_TYPE) {
            zn->_tp._transport._multicast._session = zn;
        } else
#endif  // Z_MULTICAST_TRANSPORT == 1
        {
            // Do nothing. Required to be here because of the #if directive
        }
    } else {
        _z_transport_clear(&zn->_tp);
    }

    return ret;
}

void _z_session_clear(_z_session_t *zn) {
    // Clear Zenoh PID

    // Clean up transports
    _z_transport_clear(&zn->_tp);

    // Clean up the entities
    _z_flush_resources(zn);
    _z_flush_subscriptions(zn);
    _z_flush_questionables(zn);
    _z_flush_pending_queries(zn);

#if Z_MULTI_THREAD == 1
    _z_mutex_free(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1
}

void _z_session_free(_z_session_t **zn) {
    _z_session_t *ptr = *zn;

    if (ptr != NULL) {
        _z_session_clear(ptr);

        z_free(ptr);
        *zn = NULL;
    }
}

int8_t _z_session_close(_z_session_t *zn, uint8_t reason) {
    int8_t ret = _Z_ERR_GENERIC;

    if (zn != NULL) {
        ret = _z_transport_close(&zn->_tp, reason);
    }

    return ret;
}
