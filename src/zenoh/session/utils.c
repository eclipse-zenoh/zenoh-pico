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

#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/link/manager.h"

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
        _zn_socket_result_t r_sock = zn->link->open_f(zn->link, 0);
        if (r_sock.tag == _z_res_t_OK)
            break;
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
    zn->local_resources = NULL;
    zn->remote_resources = NULL;

    zn->local_subscriptions = NULL;
    zn->remote_subscriptions = NULL;
    zn->rem_res_loc_sub_map = _z_int_void_map_make(_Z_DEFAULT_I_MAP_CAPACITY);

    zn->local_queryables = NULL;
    zn->rem_res_loc_qle_map = _z_int_void_map_make(_Z_DEFAULT_I_MAP_CAPACITY);

    zn->pending_queries = NULL;

    zn->read_task_running = 0;
    zn->read_task = NULL;

    zn->received = 0;
    zn->transmitted = 0;
    zn->lease_task_running = 0;
    zn->lease_task = NULL;

    zn->on_disconnect = &_zn_default_on_disconnect;

    return zn;
}

void _zn_session_free(zn_session_t **zn)
{
    zn_session_t *ptr = *zn;

    // Clean up link
    _zn_link_free(&ptr->link);

    // Clean up the entities
    _zn_flush_resources(ptr);
    _zn_flush_subscriptions(ptr);
    _zn_flush_queryables(ptr);
    _zn_flush_pending_queries(ptr);

    // Clean up the mutexes
    z_mutex_free(&ptr->mutex_inner);
    z_mutex_free(&ptr->mutex_tx);
    z_mutex_free(&ptr->mutex_rx);

    // Clean up the buffers
    _z_wbuf_free(&ptr->wbuf);
    _z_zbuf_free(&ptr->zbuf);

    _z_wbuf_free(&ptr->dbuf_reliable);
    _z_wbuf_free(&ptr->dbuf_best_effort);

    // Clean up the PIDs
    _z_bytes_free(&ptr->local_pid);
    _z_bytes_free(&ptr->remote_pid);

    // Clean up the locator
    free(ptr->locator);

    // Clean up the tasks
    free(ptr->read_task);
    free(ptr->lease_task);

    free(ptr);
    *zn = NULL;
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
    _zn_session_free(&zn);

    return res;
}
