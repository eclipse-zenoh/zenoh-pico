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
    _z_reply_data_clear(&reply->_data);
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
    return one->_tstamp._time == two->_tstamp._time;
}

void _z_pending_reply_clear(_z_pending_reply_t *pr)
{
    // Free reply
    _z_reply_free(&pr->_reply);

    // Free the timestamp
    _z_bytes_clear(&pr->_tstamp._id);
}

void _z_pending_query_clear(_z_pending_query_t *pen_qry)
{
    _z_str_clear(pen_qry->_rname);
    _z_keyexpr_clear(&pen_qry->_key);
    _z_str_clear(pen_qry->_predicate);

    _z_pending_reply_list_free(&pen_qry->_pending_replies);
}

int _z_pending_query_eq(const _z_pending_query_t *one, const _z_pending_query_t *two)
{
    return one->_id == two->_id;
}

/*------------------ Query ------------------*/
_z_zint_t _z_get_query_id(_z_session_t *zn)
{
    return zn->_query_id++;
}

_z_pending_query_t *__z_get_pending_query_by_id(_z_pending_query_list_t *pqls, const _z_zint_t id)
{
    _z_pending_query_t *pql = NULL;
    while (pqls != NULL)
    {
        pql = _z_pending_query_list_head(pqls);
        if (pql->_id == id)
            return pql;

        pqls = _z_pending_query_list_tail(pqls);
    }

    return NULL;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_pending_query_t *__unsafe__z_get_pending_query_by_id(_z_session_t *zn, const _z_zint_t id)
{
    _z_pending_query_list_t *pqls = zn->_pending_queries;
    return __z_get_pending_query_by_id(pqls, id);
}

_z_pending_query_t *_z_get_pending_query_by_id(_z_session_t *zn, const _z_zint_t id)
{
    _z_mutex_lock(&zn->_mutex_inner);
    _z_pending_query_t *pql = __unsafe__z_get_pending_query_by_id(zn, id);
    _z_mutex_unlock(&zn->_mutex_inner);
    return pql;
}

int _z_register_pending_query(_z_session_t *zn, _z_pending_query_t *pen_qry)
{
    _Z_DEBUG(">>> Allocating query for (%lu,%s,%s)\n", pen_qry->_key.rid, pen_qry->_key.rname, pen_qry->_predicate);
    _z_mutex_lock(&zn->_mutex_inner);

    _z_pending_query_t *pql = __unsafe__z_get_pending_query_by_id(zn, pen_qry->_id);
    if (pql != NULL) // A query for this id already exists
        goto ERR;

    // Register the query
    zn->_pending_queries = _z_pending_query_list_push(zn->_pending_queries, pen_qry);

    _z_mutex_unlock(&zn->_mutex_inner);
    return 0;

ERR:
    return -1;
}

int _z_trigger_query_reply_partial(_z_session_t *zn,
                                     const _z_reply_context_t *reply_context,
                                     const _z_keyexpr_t keyexpr,
                                     const _z_bytes_t payload,
                                     const _z_data_info_t data_info)
{
    _z_mutex_lock(&zn->_mutex_inner);

    if (_Z_HAS_FLAG(reply_context->_header, _Z_FLAG_Z_F))
        goto ERR_1;

    _z_pending_query_t *pen_qry = __unsafe__z_get_pending_query_by_id(zn, reply_context->_qid);
    if (pen_qry == NULL)
        goto ERR_1;

    // Partial reply received from an unknown target
    if (pen_qry->_target._kind != Z_QUERYABLE_ALL_KINDS && (pen_qry->_target._kind & reply_context->_replier_kind) == 0)
        goto ERR_1;

    // Take the right timestamp, or default to none
    _z_timestamp_t ts;
    if _Z_HAS_FLAG (data_info._flags, _Z_DATA_INFO_TSTAMP)
        _z_timestamp_duplicate(&ts);
    else
        _z_timestamp_reset(&ts);

    // Build the reply
    _z_reply_t *reply = (_z_reply_t *)malloc(sizeof(_z_reply_t));
    reply->_tag = Z_REPLY_TAG_DATA;
    _z_bytes_copy(&reply->_data._replier_id, &reply_context->_replier_id);
    reply->_data._replier_kind = reply_context->_replier_kind;
    if (keyexpr._rid == Z_RESOURCE_ID_NONE)
        reply->_data._sample._key = _z_keyexpr_duplicate(&keyexpr);
    else
    {
        reply->_data._sample._key._rid = Z_RESOURCE_ID_NONE;
        reply->_data._sample._key._rname = __unsafe_z_get_resource_name_from_key(zn, _Z_RESOURCE_REMOTE, &keyexpr);
    }
    _z_bytes_copy(&reply->_data._sample._value, &payload);
    reply->_data._sample._encoding._prefix = data_info._encoding._prefix;
    reply->_data._sample._encoding._suffix = data_info._encoding._suffix ? _z_str_clone(data_info._encoding._suffix) : NULL;

    // Verify if this is a newer reply, free the old one in case it is
    if (pen_qry->_consolidation._reception == Z_CONSOLIDATION_MODE_FULL || pen_qry->_consolidation._reception == Z_CONSOLIDATION_MODE_LAZY)
    {
        _z_pending_reply_list_t *pen_rps = pen_qry->_pending_replies;
        _z_pending_reply_t *pen_rep = NULL;
        while (pen_rps != NULL)
        {
            pen_rep = _z_pending_reply_list_head(pen_rps);

            // Check if this is the same resource key
            if (_z_str_eq(pen_rep->_reply->_data._sample._key._rname, reply->_data._sample._key._rname))
            {
                if (ts._time <= pen_rep->_tstamp._time)
                    goto ERR_2;
                else
                {
                    pen_qry->_pending_replies = _z_pending_reply_list_drop_filter(pen_qry->_pending_replies, _z_pending_reply_eq, pen_rep);
                    break;
                }
            }

            pen_rps = _z_pending_reply_list_tail(pen_rps);
        }

        pen_rep = (_z_pending_reply_t *)malloc(sizeof(_z_pending_reply_t));
        pen_rep->_reply = reply;
        pen_rep->_tstamp = ts;

        // Trigger the handler
        if (pen_qry->_consolidation._reception == Z_CONSOLIDATION_MODE_LAZY)
            pen_qry->_callback(*pen_rep->_reply, pen_qry->_arg);
        else
            pen_qry->_pending_replies = _z_pending_reply_list_push(pen_qry->_pending_replies, pen_rep);
    }
    else if (pen_qry->_consolidation._reception == Z_CONSOLIDATION_MODE_NONE)
    {
        pen_qry->_callback(*reply, pen_qry->_arg);
        _z_reply_free(&reply);
    }

    _z_mutex_unlock(&zn->_mutex_inner);
    return 0;

ERR_2:
    _z_reply_free(&reply);
ERR_1:
    _z_mutex_unlock(&zn->_mutex_inner);
    return -1;
}

int _z_trigger_query_reply_final(_z_session_t *zn, const _z_reply_context_t *reply_context)
{
    _z_mutex_lock(&zn->_mutex_inner);

    // Final reply received with invalid final flag
    if (!_Z_HAS_FLAG(reply_context->_header, _Z_FLAG_Z_F))
        goto ERR;

    // Final reply received for unknown query id
    _z_pending_query_t *pen_qry = __unsafe__z_get_pending_query_by_id(zn, reply_context->_qid);
    if (pen_qry == NULL)
        goto ERR;

    // Final reply received from an unknown target
    if (pen_qry->_target._kind != Z_QUERYABLE_ALL_KINDS && (pen_qry->_target._kind & reply_context->_replier_kind) == 0)
        goto ERR;

    // The reply is the final one, apply consolidation if needed
    if (pen_qry->_consolidation._reception == Z_CONSOLIDATION_MODE_FULL)
    {
        _z_pending_reply_list_t *pen_rps = pen_qry->_pending_replies;
        _z_pending_reply_t *pen_rep = NULL;
        while (pen_rps != NULL)
        {
            pen_rep = _z_pending_reply_list_head(pen_rps);

            // Check if this is the same resource key
            // Trigger the query handler
            if (_z_rname_intersect(pen_qry->_rname, pen_rep->_reply->_data._sample._key._rname))
                pen_qry->_callback(*pen_rep->_reply, pen_qry->_arg);

            pen_rps = _z_pending_reply_list_tail(pen_rps);
        }
    }

    // Trigger the final query handler
    _z_reply_t freply;
    memset(&freply, 0, sizeof(_z_reply_t));
    freply._tag = Z_REPLY_TAG_FINAL;
    pen_qry->_callback(freply, pen_qry->_arg);

    zn->_pending_queries = _z_pending_query_list_drop_filter(zn->_pending_queries, _z_pending_query_eq, pen_qry);

    _z_mutex_unlock(&zn->_mutex_inner);
    return 0;

ERR:
    _z_mutex_unlock(&zn->_mutex_inner);
    return -1;
}

void _z_unregister_pending_query(_z_session_t *zn, _z_pending_query_t *pen_qry)
{
    _z_mutex_lock(&zn->_mutex_inner);
    zn->_pending_queries = _z_pending_query_list_drop_filter(zn->_pending_queries, _z_pending_query_eq, pen_qry);
    _z_mutex_unlock(&zn->_mutex_inner);
}

void _z_flush_pending_queries(_z_session_t *zn)
{
    _z_mutex_lock(&zn->_mutex_inner);
    _z_pending_query_list_free(&zn->_pending_queries);
    _z_mutex_unlock(&zn->_mutex_inner);
}
