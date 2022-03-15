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

#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/utils/logging.h"

void _zn_reply_clear(zn_reply_t *reply)
{
    _z_str_clear(reply->data.data.key.val);
    _z_bytes_clear(&reply->data.data.value);
    _z_bytes_clear(&reply->data.replier_id);
}

void _zn_reply_free(zn_reply_t **reply)
{
    zn_reply_t *ptr = *reply;
    _zn_reply_clear(ptr);

    free(ptr);
    *reply = NULL;
}

int _zn_pending_reply_eq(const _zn_pending_reply_t *one, const _zn_pending_reply_t *two)
{
    return one->tstamp.time == two->tstamp.time; // FIXME: better comparison
}

void _zn_pending_reply_clear(_zn_pending_reply_t *pr)
{
    // Free reply
    _zn_reply_free(&pr->reply);

    // Free the timestamp
    _z_bytes_clear(&pr->tstamp.id);
}

void _zn_pending_query_clear(_zn_pending_query_t *pen_qry)
{
    _zn_reskey_clear(&pen_qry->key);
    _z_str_clear(pen_qry->predicate);

    _zn_pending_reply_list_free(&pen_qry->pending_replies);
}

int _zn_pending_query_eq(const _zn_pending_query_t *one, const _zn_pending_query_t *two)
{
    return one->id == two->id; // FIXME: better comparison
}

/*------------------ Query ------------------*/
z_zint_t _zn_get_query_id(zn_session_t *zn)
{
    return zn->query_id++;
}

_zn_pending_query_t *__zn_get_pending_query_by_id(_zn_pending_query_list_t *pqls, const z_zint_t id)
{
    _zn_pending_query_t *pql = NULL;
    while (pqls != NULL)
    {
        pql = _zn_pending_query_list_head(pqls);
        if (pql->id == id)
            return pql;

        pqls = _zn_pending_query_list_tail(pqls);
    }

    return NULL;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_pending_query_t *__unsafe_zn_get_pending_query_by_id(zn_session_t *zn, const z_zint_t id)
{
    _zn_pending_query_list_t *pqls = zn->pending_queries;
    return __zn_get_pending_query_by_id(pqls, id);
}

_zn_pending_query_t *_zn_get_pending_query_by_id(zn_session_t *zn, const z_zint_t id)
{
    z_mutex_lock(&zn->mutex_inner);
    _zn_pending_query_t *pql = __unsafe_zn_get_pending_query_by_id(zn, id);
    z_mutex_unlock(&zn->mutex_inner);
    return pql;
}

int _zn_register_pending_query(zn_session_t *zn, _zn_pending_query_t *pen_qry)
{
    _Z_DEBUG(">>> Allocating query for (%lu,%s,%s)\n", pen_qry->key.rid, pen_qry->key.rname, pen_qry->predicate);
    z_mutex_lock(&zn->mutex_inner);

    _zn_pending_query_t *pql = __unsafe_zn_get_pending_query_by_id(zn, pen_qry->id);
    if (pql != NULL) // A query for this id already exists
        goto ERR;

    // Register the query
    zn->pending_queries = _zn_pending_query_list_push(zn->pending_queries, pen_qry);

    z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR:
    return -1;
}

int _zn_trigger_query_reply_partial(zn_session_t *zn,
                                     const _zn_reply_context_t *reply_context,
                                     const zn_reskey_t reskey,
                                     const z_bytes_t payload,
                                     const _zn_data_info_t data_info)
{
    z_mutex_lock(&zn->mutex_inner);

    if (_ZN_HAS_FLAG(reply_context->header, _ZN_FLAG_Z_F))
        goto ERR_1;

    _zn_pending_query_t *pen_qry = __unsafe_zn_get_pending_query_by_id(zn, reply_context->qid);
    if (pen_qry == NULL)
        goto ERR_1;

    // Partial reply received from an unknown target
    if (pen_qry->target.kind != ZN_QUERYABLE_ALL_KINDS && (pen_qry->target.kind & reply_context->replier_kind) == 0)
        goto ERR_1;

    // Take the right timestamp, or default to none
    z_timestamp_t ts;
    if _ZN_HAS_FLAG (data_info.flags, _ZN_DATA_INFO_TSTAMP)
        z_timestamp_duplicate(&ts);
    else
        z_timestamp_reset(&ts);

    // Build the reply
    zn_reply_t *reply = (zn_reply_t *)malloc(sizeof(zn_reply_t));
    reply->tag = zn_reply_t_Tag_DATA;
    _z_bytes_copy(&reply->data.data.value, &payload);
    if (reskey.rid == ZN_RESOURCE_ID_NONE)
        reply->data.data.key.val = _z_str_clone(reskey.rname);
    else
        reply->data.data.key.val = __unsafe_zn_get_resource_name_from_key(zn, _ZN_RESOURCE_REMOTE, &reskey);
    reply->data.data.key.len = strlen(reply->data.data.key.val);
    _z_bytes_copy(&reply->data.replier_id, &reply_context->replier_id);
    reply->data.replier_kind = reply_context->replier_kind;

    // Verify if this is a newer reply, free the old one in case it is
    if (pen_qry->consolidation.reception == zn_consolidation_mode_t_FULL || pen_qry->consolidation.reception == zn_consolidation_mode_t_LAZY)
    {
        _zn_pending_reply_list_t *pen_rps = pen_qry->pending_replies;
        _zn_pending_reply_t *pen_rep = NULL;
        while (pen_rps != NULL)
        {
            pen_rep = _zn_pending_reply_list_head(pen_rps);

            // Check if this is the same resource key
            if (_z_str_eq(pen_rep->reply->data.data.key.val, reply->data.data.key.val))
            {
                if (ts.time <= pen_rep->tstamp.time)
                    goto ERR_2;
                else
                {
                    pen_qry->pending_replies = _zn_pending_reply_list_drop_filter(pen_qry->pending_replies, _zn_pending_reply_eq, pen_rep);
                    break;
                }
            }

            pen_rps = _zn_pending_reply_list_tail(pen_rps);
        }

        pen_rep = (_zn_pending_reply_t *)malloc(sizeof(_zn_pending_reply_t));
        pen_rep->reply = reply;
        pen_rep->tstamp = ts;

        // Trigger the handler
        if (pen_qry->consolidation.reception == zn_consolidation_mode_t_LAZY)
            pen_qry->callback(*pen_rep->reply, pen_qry->arg);
        else
            pen_qry->pending_replies = _zn_pending_reply_list_push(pen_qry->pending_replies, pen_rep);
    }
    else if (pen_qry->consolidation.reception == zn_consolidation_mode_t_NONE)
    {
        pen_qry->callback(*reply, pen_qry->arg);
        _zn_reply_free(&reply);
    }

    z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR_2:
    _zn_reply_free(&reply);
ERR_1:
    z_mutex_unlock(&zn->mutex_inner);
    return -1;
}

int _zn_trigger_query_reply_final(zn_session_t *zn, const _zn_reply_context_t *reply_context)
{
    z_mutex_lock(&zn->mutex_inner);

    // Final reply received with invalid final flag
    if (!_ZN_HAS_FLAG(reply_context->header, _ZN_FLAG_Z_F))
        goto ERR;

    // Final reply received for unknown query id
    _zn_pending_query_t *pen_qry = __unsafe_zn_get_pending_query_by_id(zn, reply_context->qid);
    if (pen_qry == NULL)
        goto ERR;

    // Final reply received from an unknown target
    if (pen_qry->target.kind != ZN_QUERYABLE_ALL_KINDS && (pen_qry->target.kind & reply_context->replier_kind) == 0)
        goto ERR;

    // The reply is the final one, apply consolidation if needed
    if (pen_qry->consolidation.reception == zn_consolidation_mode_t_FULL)
    {
        z_str_t rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_RESOURCE_REMOTE, &pen_qry->key);

        _zn_pending_reply_list_t *pen_rps = pen_qry->pending_replies;
        _zn_pending_reply_t *pen_rep = NULL;
        while (pen_rps != NULL)
        {
            pen_rep = _zn_pending_reply_list_head(pen_rps);

            // Check if this is the same resource key
            // Trigger the query handler
            if (zn_rname_intersect(rname, pen_rep->reply->data.data.key.val))
                pen_qry->callback(*pen_rep->reply, pen_qry->arg);

            pen_rps = _zn_pending_reply_list_tail(pen_rps);
        }

        _z_str_clear(rname);
    }

    // Trigger the final query handler
    zn_reply_t freply;
    memset(&freply, 0, sizeof(zn_reply_t));
    freply.tag = zn_reply_t_Tag_FINAL;
    pen_qry->callback(freply, pen_qry->arg);

    zn->pending_queries = _zn_pending_query_list_drop_filter(zn->pending_queries, _zn_pending_query_eq, pen_qry);

    z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR:
    z_mutex_unlock(&zn->mutex_inner);
    return -1;
}

void _zn_unregister_pending_query(zn_session_t *zn, _zn_pending_query_t *pen_qry)
{
    z_mutex_lock(&zn->mutex_inner);
    zn->pending_queries = _zn_pending_query_list_drop_filter(zn->pending_queries, _zn_pending_query_eq, pen_qry);
    z_mutex_unlock(&zn->mutex_inner);
}

void _zn_flush_pending_queries(zn_session_t *zn)
{
    z_mutex_lock(&zn->mutex_inner);
    _zn_pending_query_list_free(&zn->pending_queries);
    z_mutex_unlock(&zn->mutex_inner);
}
