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

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/utils/logging.h"

int _zn_queryable_eq(const _zn_queryable_t *one, const _zn_queryable_t *two)
{
    return one->id == two->id;
}

void _zn_queryable_clear(_zn_queryable_t *qle)
{
    _z_str_clear(qle->rname);
}

/*------------------ Queryable ------------------*/
_zn_queryable_t *__zn_get_queryable_by_id(_zn_queryable_list_t *qles, const z_zint_t id)
{
    _zn_queryable_t *qle = NULL;
    while (qles != NULL)
    {
        qle = _zn_queryable_list_head(qles);
        if (id == qle->id)
            return qle;

        qles = _zn_queryable_list_tail(qles);
    }

    return qle;
}

_zn_queryable_list_t *__zn_get_queryables_by_name(_zn_queryable_list_t *qles, const z_str_t rname)
{
    _zn_queryable_list_t *xs = NULL;
    while (qles != NULL)
    {
        _zn_queryable_t *qle = _zn_queryable_list_head(qles);
        if (zn_rname_intersect(qle->rname, rname))
            xs = _zn_queryable_list_push(xs, qle);

        qles = _zn_queryable_list_tail(qles);
    }

    return xs;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_queryable_t *__unsafe_zn_get_queryable_by_id(zn_session_t *zn, const z_zint_t id)
{
    _zn_queryable_list_t *qles = zn->local_queryables;
    return __zn_get_queryable_by_id(qles, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_queryable_list_t *__unsafe_zn_get_queryables_by_name(zn_session_t *zn, const z_str_t rname)
{
    _zn_queryable_list_t *qles = zn->local_queryables;
    return __zn_get_queryables_by_name(qles, rname);
}

_zn_queryable_t *_zn_get_queryable_by_id(zn_session_t *zn, const z_zint_t id)
{
    z_mutex_lock(&zn->mutex_inner);
    _zn_queryable_t *qle = __unsafe_zn_get_queryable_by_id(zn, id);
    z_mutex_unlock(&zn->mutex_inner);
    return qle;
}

_zn_queryable_list_t *_zn_get_queryables_by_name(zn_session_t *zn, const z_str_t rname)
{
    z_mutex_lock(&zn->mutex_inner);
    _zn_queryable_list_t *qles = __unsafe_zn_get_queryables_by_name(zn, rname);
    z_mutex_unlock(&zn->mutex_inner);
    return qles;
}

_zn_queryable_list_t *_zn_get_queryables_by_key(zn_session_t *zn, const zn_reskey_t *reskey)
{
    z_mutex_lock(&zn->mutex_inner);
    z_str_t rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, reskey);
    _zn_queryable_list_t *qles = __unsafe_zn_get_queryables_by_name(zn, rname);
    z_mutex_unlock(&zn->mutex_inner);

    _z_str_clear(rname);
    return qles;
}

int _zn_register_queryable(zn_session_t *zn, _zn_queryable_t *qle)
{
    _Z_DEBUG_VA(">>> Allocating queryable for (%s,%u)\n", qle->rname, qle->kind);
    z_mutex_lock(&zn->mutex_inner);

    zn->local_queryables = _zn_queryable_list_push(zn->local_queryables, qle);

    z_mutex_unlock(&zn->mutex_inner);
    return 0;
}

int _zn_trigger_queryables(zn_session_t *zn, const _zn_query_t *query)
{
    z_mutex_lock(&zn->mutex_inner);

    z_str_t rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_REMOTE, &query->key);
    if(rname == NULL)
        goto ERR;

    // Build the query
    zn_query_t q;
    q.zn = zn;
    q.qid = query->qid;
    q.rname = rname;
    q.predicate = query->predicate;

    _zn_queryable_list_t *qles = __unsafe_zn_get_queryables_by_name(zn, rname);
    _zn_queryable_list_t *xs = qles;
    while (xs != NULL)
    {
        _zn_queryable_t *qle = _zn_queryable_list_head(xs);
        qle->callback(&q, qle->arg);

        // Send the final reply
        z_bytes_t pid;
        _z_bytes_reset(&pid);

        z_zint_t kind = 0;
        int is_final = 1;

        _zn_reply_context_t *rctx = _zn_z_msg_make_reply_context(query->qid, pid, kind, is_final);

        // Congestion control
        int can_be_dropped = 0;

        // Create the final reply
        _zn_zenoh_message_t z_msg = _zn_z_msg_make_unit(can_be_dropped);
        z_msg.reply_context = rctx;

        if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
        {
            // @TODO: retransmission
        }

        _zn_z_msg_clear(&z_msg);
        xs = _zn_queryable_list_tail(xs);
    }

    _z_str_clear(rname);
    _z_list_free(&qles, _zn_noop_free);
    z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR:
    z_mutex_unlock(&zn->mutex_inner);
    return -1;
}

void _zn_unregister_queryable(zn_session_t *zn, _zn_queryable_t *qle)
{
    z_mutex_lock(&zn->mutex_inner);
    zn->local_queryables = _zn_queryable_list_drop_filter(zn->local_queryables, _zn_queryable_eq, qle);
    z_mutex_unlock(&zn->mutex_inner);
}

void _zn_flush_queryables(zn_session_t *zn)
{
    z_mutex_lock(&zn->mutex_inner);
    _zn_queryable_list_free(&zn->local_queryables);
    z_mutex_unlock(&zn->mutex_inner);
}
