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
#include "zenoh-pico/net/memory.h"

void _z_reply_clear(_z_reply_t *reply)
{
    _z_reply_data_clear(&reply->data);
}

void _z_reply_free(_z_reply_t **reply)
{
    _z_reply_t *ptr = *reply;
    _z_reply_clear(ptr);

    free(ptr);
    *reply = NULL;
}

int _z_pending_reply_eq(const _z_pending_reply_t *one, const _z_pending_reply_t *two)
{
    return one->tstamp.time == two->tstamp.time;
}

void _z_pending_reply_clear(_z_pending_reply_t *pr)
{
    // Free reply
    _z_reply_free(&pr->reply);

    // Free the timestamp
    _z_bytes_clear(&pr->tstamp.id);
}

void _z_pending_query_clear(_z_pending_query_t *pen_qry)
{
    _z_str_clear(pen_qry->rname);
    _z_reskey_clear(&pen_qry->key);
    _z_str_clear(pen_qry->predicate);

    _z_pending_reply_list_free(&pen_qry->pending_replies);
}

int _z_pending_query_eq(const _z_pending_query_t *one, const _z_pending_query_t *two)
{
    return one->id == two->id;
}

/*------------------ Query ------------------*/
_z_zint_t _z_get_query_id(_z_session_t *zn)
{
    return zn->query_id++;
}

_z_pending_query_t *__z_get_pending_query_by_id(_z_pending_query_list_t *pqls, const _z_zint_t id)
{
    _z_pending_query_t *pql = NULL;
    while (pqls != NULL)
    {
        pql = _z_pending_query_list_head(pqls);
        if (pql->id == id)
            return pql;

        pqls = _z_pending_query_list_tail(pqls);
    }

    return NULL;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_z_pending_query_t *__unsafe__z_get_pending_query_by_id(_z_session_t *zn, const _z_zint_t id)
{
    _z_pending_query_list_t *pqls = zn->pending_queries;
    return __z_get_pending_query_by_id(pqls, id);
}

_z_pending_query_t *_z_get_pending_query_by_id(_z_session_t *zn, const _z_zint_t id)
{
    _z_mutex_lock(&zn->mutex_inner);
    _z_pending_query_t *pql = __unsafe__z_get_pending_query_by_id(zn, id);
    _z_mutex_unlock(&zn->mutex_inner);
    return pql;
}

int _z_register_pending_query(_z_session_t *zn, _z_pending_query_t *pen_qry)
{
    _Z_DEBUG(">>> Allocating query for (%lu,%s,%s)\n", pen_qry->key.rid, pen_qry->key.rname, pen_qry->predicate);
    _z_mutex_lock(&zn->mutex_inner);

    _z_pending_query_t *pql = __unsafe__z_get_pending_query_by_id(zn, pen_qry->id);
    if (pql != NULL) // A query for this id already exists
        goto ERR;

    // Register the query
    zn->pending_queries = _z_pending_query_list_push(zn->pending_queries, pen_qry);

    _z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR:
    return -1;
}

int _z_trigger_query_reply_partial(_z_session_t *zn,
                                     const _z_reply_context_t *reply_context,
                                     const _z_reskey_t reskey,
                                     const _z_bytes_t payload,
                                     const _z_data_info_t data_info)
{
    _z_mutex_lock(&zn->mutex_inner);

    if (_Z_HAS_FLAG(reply_context->header, _Z_FLAG_Z_F))
        goto ERR_1;

    _z_pending_query_t *pen_qry = __unsafe__z_get_pending_query_by_id(zn, reply_context->qid);
    if (pen_qry == NULL)
        goto ERR_1;

    // Partial reply received from an unknown target
    if (pen_qry->target.kind != Z_QUERYABLE_ALL_KINDS && (pen_qry->target.kind & reply_context->replier_kind) == 0)
        goto ERR_1;

    // Take the right timestamp, or default to none
    _z_timestamp_t ts;
    if _Z_HAS_FLAG (data_info.flags, _Z_DATA_INFO_TSTAMP)
        _z_timestamp_duplicate(&ts);
    else
        _z_timestamp_reset(&ts);

    // Build the reply
    _z_reply_t *reply = (_z_reply_t *)malloc(sizeof(_z_reply_t));
    reply->tag = Z_REPLY_TAG_DATA;
    _z_bytes_copy(&reply->data.replier_id, &reply_context->replier_id);
    reply->data.replier_kind = reply_context->replier_kind;
    if (reskey.rid == Z_RESOURCE_ID_NONE)
        reply->data.data.key = _z_reskey_duplicate(&reskey);
    else
    {
        reply->data.data.key.rid = Z_RESOURCE_ID_NONE;
        reply->data.data.key.rname = __unsafe_z_get_resource_name_from_key(zn, _Z_RESOURCE_REMOTE, &reskey);
    }
    _z_bytes_copy(&reply->data.data.value, &payload);
    reply->data.data.encoding.prefix = data_info.encoding.prefix;
    reply->data.data.encoding.suffix = data_info.encoding.suffix ? _z_str_clone(data_info.encoding.suffix) : NULL;

    // Verify if this is a newer reply, free the old one in case it is
    if (pen_qry->consolidation.reception == Z_CONSOLIDATION_MODE_FULL || pen_qry->consolidation.reception == Z_CONSOLIDATION_MODE_LAZY)
    {
        _z_pending_reply_list_t *pen_rps = pen_qry->pending_replies;
        _z_pending_reply_t *pen_rep = NULL;
        while (pen_rps != NULL)
        {
            pen_rep = _z_pending_reply_list_head(pen_rps);

            // Check if this is the same resource key
            if (_z_str_eq(pen_rep->reply->data.data.key.rname, reply->data.data.key.rname))
            {
                if (ts.time <= pen_rep->tstamp.time)
                    goto ERR_2;
                else
                {
                    pen_qry->pending_replies = _z_pending_reply_list_drop_filter(pen_qry->pending_replies, _z_pending_reply_eq, pen_rep);
                    break;
                }
            }

            pen_rps = _z_pending_reply_list_tail(pen_rps);
        }

        pen_rep = (_z_pending_reply_t *)malloc(sizeof(_z_pending_reply_t));
        pen_rep->reply = reply;
        pen_rep->tstamp = ts;

        // Trigger the handler
        if (pen_qry->consolidation.reception == Z_CONSOLIDATION_MODE_LAZY)
            pen_qry->callback(*pen_rep->reply, pen_qry->arg);
        else
            pen_qry->pending_replies = _z_pending_reply_list_push(pen_qry->pending_replies, pen_rep);
    }
    else if (pen_qry->consolidation.reception == Z_CONSOLIDATION_MODE_NONE)
    {
        pen_qry->callback(*reply, pen_qry->arg);
        _z_reply_free(&reply);
    }

    _z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR_2:
    _z_reply_free(&reply);
ERR_1:
    _z_mutex_unlock(&zn->mutex_inner);
    return -1;
}

int _z_trigger_query_reply_final(_z_session_t *zn, const _z_reply_context_t *reply_context)
{
    _z_mutex_lock(&zn->mutex_inner);

    // Final reply received with invalid final flag
    if (!_Z_HAS_FLAG(reply_context->header, _Z_FLAG_Z_F))
        goto ERR;

    // Final reply received for unknown query id
    _z_pending_query_t *pen_qry = __unsafe__z_get_pending_query_by_id(zn, reply_context->qid);
    if (pen_qry == NULL)
        goto ERR;

    // Final reply received from an unknown target
    if (pen_qry->target.kind != Z_QUERYABLE_ALL_KINDS && (pen_qry->target.kind & reply_context->replier_kind) == 0)
        goto ERR;

    // The reply is the final one, apply consolidation if needed
    if (pen_qry->consolidation.reception == Z_CONSOLIDATION_MODE_FULL)
    {
        _z_pending_reply_list_t *pen_rps = pen_qry->pending_replies;
        _z_pending_reply_t *pen_rep = NULL;
        while (pen_rps != NULL)
        {
            pen_rep = _z_pending_reply_list_head(pen_rps);

            // Check if this is the same resource key
            // Trigger the query handler
            if (_z_rname_intersect(pen_qry->rname, pen_rep->reply->data.data.key.rname))
                pen_qry->callback(*pen_rep->reply, pen_qry->arg);

            pen_rps = _z_pending_reply_list_tail(pen_rps);
        }
    }

    // Trigger the final query handler
    _z_reply_t freply;
    memset(&freply, 0, sizeof(_z_reply_t));
    freply.tag = Z_REPLY_TAG_FINAL;
    pen_qry->callback(freply, pen_qry->arg);

    zn->pending_queries = _z_pending_query_list_drop_filter(zn->pending_queries, _z_pending_query_eq, pen_qry);

    _z_mutex_unlock(&zn->mutex_inner);
    return 0;

ERR:
    _z_mutex_unlock(&zn->mutex_inner);
    return -1;
}

void _z_unregister_pending_query(_z_session_t *zn, _z_pending_query_t *pen_qry)
{
    _z_mutex_lock(&zn->mutex_inner);
    zn->pending_queries = _z_pending_query_list_drop_filter(zn->pending_queries, _z_pending_query_eq, pen_qry);
    _z_mutex_unlock(&zn->mutex_inner);
}

void _z_flush_pending_queries(_z_session_t *zn)
{
    _z_mutex_lock(&zn->mutex_inner);
    _z_pending_query_list_free(&zn->pending_queries);
    _z_mutex_unlock(&zn->mutex_inner);
}
