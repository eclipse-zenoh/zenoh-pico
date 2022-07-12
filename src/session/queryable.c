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
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/net/resource.h"
#include "zenoh-pico/utils/logging.h"

int _z_questionable_eq(const _z_questionable_t *one, const _z_questionable_t *two)
{
    return one->_id == two->_id;
}

void _z_questionable_clear(_z_questionable_t *qle)
{
    if (qle->_dropper != NULL)
        qle->_dropper(qle->_arg);
    _z_keyexpr_clear(&qle->_key);
}

/*------------------ Queryable ------------------*/
_z_questionable_t *__z_get_queryable_by_id(_z_questionable_list_t *qles, const _z_zint_t id)
{
    _z_questionable_t *qle = NULL;
    while (qles != NULL)
    {
        qle = _z_questionable_list_head(qles);
        if (id == qle->_id)
            return qle;

        qles = _z_questionable_list_tail(qles);
    }

    return qle;
}

_z_questionable_list_t *__z_get_queryables_by_key(_z_questionable_list_t *qles, const _z_keyexpr_t key)
{
    _z_questionable_list_t *xs = NULL;
    while (qles != NULL)
    {
        _z_questionable_t *qle = _z_questionable_list_head(qles);
        if (_z_rname_intersect(qle->_key.suffix, key.suffix))
            xs = _z_questionable_list_push(xs, qle);

        qles = _z_questionable_list_tail(qles);
    }

    return xs;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_questionable_t *__unsafe_z_get_queryable_by_id(_z_session_t *zn, const _z_zint_t id)
{
    _z_questionable_list_t *qles = zn->_local_questionable;
    return __z_get_queryable_by_id(qles, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_questionable_list_t *__unsafe_z_get_queryables_by_key(_z_session_t *zn, const _z_keyexpr_t key)
{
    _z_questionable_list_t *qles = zn->_local_questionable;
    return __z_get_queryables_by_key(qles, key);
}

_z_questionable_t *_z_get_queryable_by_id(_z_session_t *zn, const _z_zint_t id)
{
    _z_mutex_lock(&zn->_mutex_inner);
    _z_questionable_t *qle = __unsafe_z_get_queryable_by_id(zn, id);
    _z_mutex_unlock(&zn->_mutex_inner);
    return qle;
}

_z_questionable_list_t *_z_get_queryables_by_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr)
{
    _z_mutex_lock(&zn->_mutex_inner);
    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_LOCAL, keyexpr);
    _z_questionable_list_t *qles = __unsafe_z_get_queryables_by_key(zn, key);
    _z_mutex_unlock(&zn->_mutex_inner);

    return qles;
}

int _z_register_queryable(_z_session_t *zn, _z_questionable_t *qle)
{
    _Z_DEBUG(">>> Allocating queryable for (%s,%u)\n", qle->_rname, qle->_kind);
    _z_mutex_lock(&zn->_mutex_inner);

    zn->_local_questionable = _z_questionable_list_push(zn->_local_questionable, qle);

    _z_mutex_unlock(&zn->_mutex_inner);
    return 0;
}

int _z_trigger_queryables(_z_session_t *zn, const _z_msg_query_t *query)
{
    _z_mutex_lock(&zn->_mutex_inner);

    _z_keyexpr_t key = __unsafe_z_get_expanded_key_from_key(zn, _Z_RESOURCE_REMOTE, &query->_key);
    if(key.suffix == NULL)
        goto ERR;

    // Build the query
    z_query_t q;
    q._zn = zn;
    q._qid = query->_qid;
    q.key = key;
    q.predicate = query->_predicate;

    _z_questionable_list_t *qles = __unsafe_z_get_queryables_by_key(zn, key);
    _z_questionable_list_t *xs = qles;
    while (xs != NULL)
    {
        _z_questionable_t *qle = _z_questionable_list_head(xs);
        if (((query->_target._kind & Z_QUERYABLE_ALL_KINDS) | (query->_target._kind & qle->_kind)) != 0)
        {
            q.kind = qle->_kind;
            qle->_callback(&q, qle->_arg);
        }

        xs = _z_questionable_list_tail(xs);
    }

    // Send the final reply
    // Final flagged reply context does not encode the PID or replier kind
    _z_bytes_t pid;
    _z_bytes_reset(&pid);
    _z_zint_t replier_kind = 0;
    int is_final = 1;
    _z_reply_context_t *rctx = _z_msg_make_reply_context(query->_qid, pid, replier_kind, is_final);

    // Congestion control
    int can_be_dropped = 0;

    // Create the final reply
    _z_zenoh_message_t z_msg = _z_msg_make_unit(can_be_dropped);
    z_msg._reply_context = rctx;

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }
    _z_msg_clear(&z_msg);

    _z_keyexpr_clear(&key);
    _z_list_free(&qles, _z_noop_free);
    _z_mutex_unlock(&zn->_mutex_inner);
    return 0;

ERR:
    _z_mutex_unlock(&zn->_mutex_inner);
    return -1;
}

void _z_unregister_queryable(_z_session_t *zn, _z_questionable_t *qle)
{
    _z_mutex_lock(&zn->_mutex_inner);
    zn->_local_questionable = _z_questionable_list_drop_filter(zn->_local_questionable, _z_questionable_eq, qle);
    _z_mutex_unlock(&zn->_mutex_inner);
}

void _z_flush_queryables(_z_session_t *zn)
{
    _z_mutex_lock(&zn->_mutex_inner);
    _z_questionable_list_free(&zn->_local_questionable);
    _z_mutex_unlock(&zn->_mutex_inner);
}
