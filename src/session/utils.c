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

#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/query.h"

/*------------------ clone helpers ------------------*/
_z_reskey_t _z_reskey_duplicate(const _z_reskey_t *reskey)
{
    _z_reskey_t rk;
    rk.rid = reskey->rid,
    rk.rname = reskey->rname ? _z_str_clone(reskey->rname) : NULL;
    return rk;
}

_z_timestamp_t _z_timestamp_duplicate(const _z_timestamp_t *tstamp)
{
    _z_timestamp_t ts;
    _z_bytes_copy(&ts.id, &tstamp->id);
    ts.time = tstamp->time;
    return ts;
}

void _z_timestamp_reset(_z_timestamp_t *tstamp)
{
    _z_bytes_reset(&tstamp->id);
    tstamp->time = 0;
}

/*------------------ Init/Free/Close session ------------------*/
_z_session_t *_z_session_init(void)
{
    _z_session_t *zn = (_z_session_t *)malloc(sizeof(_z_session_t));

    // Initialize the counters to 1
    zn->entity_id = 1;
    zn->resource_id = 1;
    zn->query_id = 1;
    zn->pull_id = 1;

    // Initialize the data structs
    zn->local_resources = NULL;
    zn->remote_resources = NULL;
    zn->local_subscriptions = NULL;
    zn->remote_subscriptions = NULL;
    zn->local_queryables = NULL;
    zn->pending_queries = NULL;

    // Associate a transport with the session
    zn->tp = NULL;
    zn->tp_manager = _z_transport_manager_init();

    // Initialize the mutexes
    _z_mutex_init(&zn->mutex_inner);

    return zn;
}

void _z_session_free(_z_session_t **zn)
{
    _z_session_t *ptr = *zn;

    // Clean up transports and manager
    _z_transport_manager_free(&ptr->tp_manager);
    if (ptr->tp != NULL)
        _z_transport_free(&ptr->tp);

    // Clean up the entities
    _z_flush_resources(ptr);
    _z_flush_subscriptions(ptr);
    _z_flush_queryables(ptr);
    _z_flush_pending_queries(ptr);

    // Clean up the mutexes
    _z_mutex_free(&ptr->mutex_inner);

    free(ptr);
    *zn = NULL;
}

int _z_session_close(_z_session_t *zn, uint8_t reason)
{
    int res = _z_transport_close(zn->tp, reason);

    // Free the session
    _z_session_free(&zn);

    return res;
}
