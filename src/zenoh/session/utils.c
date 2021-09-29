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

#include "zenoh-pico/protocol/private/msg.h"
#include "zenoh-pico/protocol/private/msgcodec.h"
#include "zenoh-pico/protocol/private/utils.h"
#include "zenoh-pico/system/common.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/utils/private/logging.h"
#include "zenoh-pico/system/common.h"
#include "zenoh-pico/session/types.h"
#include "zenoh-pico/session/private/resource.h"
#include "zenoh-pico/session/private/subscription.h"
#include "zenoh-pico/session/private/queryable.h"
#include "zenoh-pico/session/private/query.h"
#include "zenoh-pico/transport/private/utils.h"
#include "zenoh-pico/utils/types.h"

/*------------------ Clone helpers ------------------*/
zn_reskey_t _zn_reskey_clone(const zn_reskey_t *reskey)
{
    zn_reskey_t rk;
    rk.rid = reskey->rid,
    rk.rname = reskey->rname ? strdup(reskey->rname) : NULL;
    return rk;
}

z_timestamp_t z_timestamp_clone(const z_timestamp_t *tstamp)
{
    z_timestamp_t ts;
    _z_bytes_copy(&ts.id, &tstamp->id);
    ts.time = tstamp->time;
    return ts;
}

void z_timestamp_reset(z_timestamp_t *tstamp)
{
    _z_bytes_reset(&tstamp->id);
    tstamp->time = 0;
}

/*------------------ Init/Free/Close session ------------------*/
void _zn_default_on_disconnect(void *vz)
{
    zn_session_t *zn = (zn_session_t *)vz;
    for (int i = 0; i < 3; i++)
    {
        z_sleep_s(3);
        // Try to reconnect -- eventually we should scout here.
        // We should also re-do declarations.
        _Z_DEBUG("Tring to reconnect...\n");
        if (zn->link->o_func(zn->link) < 0)
        {
            return;
        }
    }
}

zn_session_t *_zn_session_init()
{
    zn_session_t *zn = (zn_session_t *)malloc(sizeof(zn_session_t));

    // Initialize the read and write buffers
    zn->wbuf = _z_wbuf_make(ZN_WRITE_BUF_LEN, 0);
    zn->zbuf = _z_zbuf_make(ZN_READ_BUF_LEN);

    // Initialize the defragmentation buffers
    zn->dbuf_reliable = _z_wbuf_make(0, 1);
    zn->dbuf_best_effort = _z_wbuf_make(0, 1);

    // Initialize the mutexes
    z_mutex_init(&zn->mutex_rx);
    z_mutex_init(&zn->mutex_tx);
    z_mutex_init(&zn->mutex_inner);

    // The initial SN at RX side
    zn->lease = 0;
    zn->sn_resolution = 0;

    // The initial SN at RX side
    zn->sn_rx_reliable = 0;
    zn->sn_rx_best_effort = 0;

    // The initial SN at TX side
    zn->sn_tx_reliable = 0;
    zn->sn_tx_best_effort = 0;

    // Initialize the counters to 1
    zn->entity_id = 1;
    zn->resource_id = 1;
    zn->query_id = 1;
    zn->pull_id = 1;

    // Initialize the data structs
    zn->local_resources = z_list_empty;
    zn->remote_resources = z_list_empty;

    zn->local_subscriptions = z_list_empty;
    zn->remote_subscriptions = z_list_empty;
    zn->rem_res_loc_sub_map = z_i_map_make(_Z_DEFAULT_I_MAP_CAPACITY);

    zn->local_queryables = z_list_empty;
    zn->rem_res_loc_qle_map = z_i_map_make(_Z_DEFAULT_I_MAP_CAPACITY);

    zn->pending_queries = z_list_empty;

    zn->read_task_running = 0;
    zn->read_task = NULL;

    zn->received = 0;
    zn->transmitted = 0;
    zn->lease_task_running = 0;
    zn->lease_task = NULL;

    zn->on_disconnect = &_zn_default_on_disconnect;

    return zn;
}

void _zn_session_free(zn_session_t *zn)
{
    // Clean up link
    zn->link->c_func(zn->link); 

    // Clean up the entities
    _zn_flush_resources(zn);
    _zn_flush_subscriptions(zn);
    _zn_flush_queryables(zn);
    _zn_flush_pending_queries(zn);

    // Clean up the mutexes
    z_mutex_free(&zn->mutex_inner);
    z_mutex_free(&zn->mutex_tx);
    z_mutex_free(&zn->mutex_rx);

    // Clean up the buffers
    _z_wbuf_free(&zn->wbuf);
    _z_zbuf_free(&zn->zbuf);

    _z_wbuf_free(&zn->dbuf_reliable);
    _z_wbuf_free(&zn->dbuf_best_effort);

    // Clean up the PIDs
    _z_bytes_free(&zn->local_pid);
    _z_bytes_free(&zn->remote_pid);

    // Clean up the locator
    free(zn->locator);

    // Clean up the tasks
    free(zn->read_task);
    free(zn->lease_task);

    free(zn);

    zn = NULL;
}

int _zn_send_close(zn_session_t *zn, uint8_t reason, int link_only)
{
    _zn_transport_message_t cm = _zn_transport_message_init(_ZN_MID_CLOSE);
    cm.body.close.pid = zn->local_pid;
    cm.body.close.reason = reason;
    _ZN_SET_FLAG(cm.header, _ZN_FLAG_T_I);
    if (link_only)
        _ZN_SET_FLAG(cm.header, _ZN_FLAG_T_K);

    int res = _zn_send_t_msg(zn, &cm);

    // Free the message
    _zn_transport_message_free(&cm);

    return res;
}

int _zn_session_close(zn_session_t *zn, uint8_t reason)
{
    int res = _zn_send_close(zn, reason, 0);

    // Free the session
    _zn_close_link(zn->link);
    _zn_session_free(zn);

    return res;
}
