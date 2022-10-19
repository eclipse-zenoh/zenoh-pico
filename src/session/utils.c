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

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"

/*------------------ clone helpers ------------------*/
void _z_keyexpr_copy(_z_keyexpr_t *dst, const _z_keyexpr_t *src) {
    dst->_id = src->_id;
    dst->_suffix = src->_suffix ? _z_str_clone(src->_suffix) : NULL;
}

_z_keyexpr_t _z_keyexpr_duplicate(const _z_keyexpr_t *src) {
    _z_keyexpr_t dst;
    _z_keyexpr_copy(&dst, src);
    return dst;
}

_z_timestamp_t _z_timestamp_duplicate(const _z_timestamp_t *tstamp) {
    _z_timestamp_t ts;
    _z_bytes_copy(&ts._id, &tstamp->_id);
    ts._time = tstamp->_time;
    return ts;
}

void _z_timestamp_reset(_z_timestamp_t *tstamp) {
    _z_bytes_reset(&tstamp->_id);
    tstamp->_time = 0;
}

/*------------------ Init/Free/Close session ------------------*/
_z_session_t *_z_session_init(void) {
    _z_session_t *zn = (_z_session_t *)z_malloc(sizeof(_z_session_t));

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

    // Associate a transport with the session
    zn->_tp = NULL;
    zn->_tp_manager = _z_transport_manager_init();

#if Z_MULTI_THREAD == 1
    // Initialize the mutexes
    _z_mutex_init(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return zn;
}

void _z_session_free(_z_session_t **zn) {
    _z_session_t *ptr = *zn;

    // Clean up transports and manager
    _z_transport_manager_free(&ptr->_tp_manager);
    if (ptr->_tp != NULL) {
        _z_transport_free(&ptr->_tp);
    }

    // Clean up the entities
    _z_flush_resources(ptr);
    _z_flush_subscriptions(ptr);
    _z_flush_questionables(ptr);
    _z_flush_pending_queries(ptr);

#if Z_MULTI_THREAD == 1
    // Clean up the mutexes
    _z_mutex_free(&ptr->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    z_free(ptr);
    *zn = NULL;
}

int8_t _z_session_close(_z_session_t *zn, uint8_t reason) {
    int8_t ret = _Z_RES_OK;

    ret = _z_transport_close(zn->_tp, reason);

    return ret;
}
