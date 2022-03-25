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

#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/logger.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/protocol/utils.h"

_z_hello_array_t _z_scout(const _z_zint_t what, const _z_properties_t *config, const uint32_t timeout)
{
    return _z_scout_inner(what, config, timeout, 0);
}

/*------------------ Resource Declaration ------------------*/
_z_zint_t _z_declare_resource(_z_session_t *zn, _z_reskey_t reskey)
{
    _z_resource_t *r = (_z_resource_t *)malloc(sizeof(_z_resource_t));
    r->id = _z_get_resource_id(zn);
    r->key = reskey;

    // FIXME: remove when resource declaration is implemented for multicast transport
    if (zn->tp->type == _Z_TRANSPORT_MULTICAST_TYPE)
        goto ERR;

    int res = _z_register_resource(zn, _Z_RESOURCE_IS_LOCAL, r);
    if (res != 0)
        goto ERR;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations.val[0] = _z_msg_make_declaration_resource(r->id, _z_reskey_duplicate(&r->key));

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);

    return r->id;

ERR:
    free(r);
    return Z_RESOURCE_ID_NONE;
}

void _z_undeclare_resource(_z_session_t *zn, const _z_zint_t rid)
{
    _z_resource_t *r = _z_get_resource_by_id(zn, _Z_RESOURCE_IS_LOCAL, rid);
    if (r == NULL)
        return;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations.val[0] = _z_msg_make_declaration_forget_resource(rid);

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);
    _z_unregister_resource(zn, _Z_RESOURCE_IS_LOCAL, r);
}

/*------------------  Publisher Declaration ------------------*/
z_publisher_t *_z_declare_publisher(_z_session_t *zn, _z_reskey_t reskey)
{
    z_publisher_t *pub = (z_publisher_t *)malloc(sizeof(z_publisher_t));
    pub->zn = zn;
    pub->key = reskey;
    pub->id = _z_get_entity_id(zn);

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations.val[0] = _z_msg_make_declaration_publisher(_z_reskey_duplicate(&reskey));

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);

    return pub;
}

void _z_undeclare_publisher(z_publisher_t *pub)
{
    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations.val[0] = _z_msg_make_declaration_forget_publisher(_z_reskey_duplicate(&pub->key));

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(pub->zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);
}

/*------------------ Subscriber Declaration ------------------*/
z_subscriber_t *_z_declare_subscriber(_z_session_t *zn, _z_reskey_t reskey, _z_subinfo_t sub_info, _z_data_handler_t callback, void *arg)
{
    _z_subscriber_t *rs = (_z_subscriber_t *)malloc(sizeof(_z_subscriber_t));
    rs->id = _z_get_entity_id(zn);
    rs->rname = _z_get_resource_name_from_key(zn, _Z_RESOURCE_IS_LOCAL, &reskey);
    rs->key = reskey;
    rs->info = sub_info;
    rs->callback = callback;
    rs->arg = arg;

    int res = _z_register_subscription(zn, _Z_RESOURCE_IS_LOCAL, rs);
    if (res != 0)
        goto ERR;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations.val[0] = _z_msg_make_declaration_subscriber(_z_reskey_duplicate(&reskey), sub_info);

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);

    z_subscriber_t *subscriber = (z_subscriber_t *)malloc(sizeof(z_subscriber_t));
    subscriber->zn = zn;
    subscriber->id = rs->id;

    return subscriber;

ERR:
    _z_str_clear(rs->rname);
    free(rs);
    return NULL;
}

void _z_undeclare_subscriber(z_subscriber_t *sub)
{
    _z_subscriber_t *s = _z_get_subscription_by_id(sub->zn, _Z_RESOURCE_IS_LOCAL, sub->id);
    if (s == NULL)
        return;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    _z_reskey_t key;
    key.rid = Z_RESOURCE_ID_NONE;
    key.rname = _z_str_clone(s->rname);
    declarations.val[0] = _z_msg_make_declaration_forget_subscriber(key);

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(sub->zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);
    _z_unregister_subscription(sub->zn, _Z_RESOURCE_IS_LOCAL, s);
}

/*------------------ Queryable Declaration ------------------*/
z_queryable_t *_z_declare_queryable(_z_session_t *zn, _z_reskey_t reskey, unsigned int kind, _z_queryable_handler_t callback, void *arg)
{
    _z_queryable_t *rq = (_z_queryable_t *)malloc(sizeof(_z_queryable_t));
    rq->id = _z_get_entity_id(zn);
    rq->rname = _z_get_resource_name_from_key(zn, _Z_RESOURCE_IS_LOCAL, &reskey);
    rq->key = reskey;
    rq->kind = kind;
    rq->callback = callback;
    rq->arg = arg;

    int res = _z_register_queryable(zn, rq);
    if (res != 0)
        goto ERR;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations.val[0] = _z_msg_make_declaration_queryable(_z_reskey_duplicate(&reskey), kind, _Z_QUERYABLE_COMPLETE_DEFAULT, _Z_QUERYABLE_DISTANCE_DEFAULT);

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);

    z_queryable_t *queryable = (z_queryable_t *)malloc(sizeof(z_queryable_t));
    queryable->zn = zn;
    queryable->id = rq->id;

    return queryable;

ERR:
    _z_str_clear(rq->rname);
    free(rq);
    return NULL;
}

void _z_undeclare_queryable(z_queryable_t *qle)
{
    _z_queryable_t *q = _z_get_queryable_by_id(qle->zn, qle->id);
    if (q == NULL)
        return;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    _z_reskey_t key;
    key.rid = Z_RESOURCE_ID_NONE;
    key.rname = _z_str_clone(q->rname);
    declarations.val[0] = _z_msg_make_declaration_forget_queryable(key, q->kind);

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(qle->zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);

    _z_unregister_queryable(qle->zn, q);
}

void _z_send_reply(const z_query_t *query, const _z_str_t key, const uint8_t *payload, const size_t len)
{
    // Build the reply context decorator. This is NOT the final reply.
    _z_bytes_t pid = _z_bytes_wrap(((_z_session_t*)query->zn)->tp_manager->local_pid.val, ((_z_session_t*)query->zn)->tp_manager->local_pid.len);
    _z_reply_context_t *rctx = _z_msg_make_reply_context(query->qid, pid, query->kind, 0);

    // @TODO: use numerical resources if possible
    // ResKey
    _z_reskey_t reskey;
    reskey.rid = Z_RESOURCE_ID_NONE;
    reskey.rname = key;

    // Empty data info
    _z_data_info_t di;
    di.flags = 0;

    // Payload
    _z_payload_t pld;
    pld.len = len;
    pld.val = payload;

    // Congestion control
    int can_be_dropped = 0;

    _z_zenoh_message_t z_msg = _z_msg_make_reply(reskey, di, pld, can_be_dropped, rctx);

    if (_z_send_z_msg(query->zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    free(rctx);
    // FIXME: Provide wrap for all allocated values, so that clear can be executed
    //        Still, currently it is not leaking, since all values are owned by the user
    //_z_msg_clear(&z_msg);
}

/*------------------ Write ------------------*/
int _z_write(_z_session_t *zn, const _z_reskey_t reskey, const uint8_t *payload, const size_t len)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    // Empty data info
    _z_data_info_t info;
    info.flags = 0;

    // Payload
    _z_payload_t pld;
    pld.len = len;
    pld.val = payload;

    // Congestion control
    int can_be_dropped = Z_CONGESTION_CONTROL_DEFAULT == Z_CONGESTION_CONTROL_DROP;

    _z_zenoh_message_t z_msg = _z_msg_make_data(reskey, info, pld, can_be_dropped);

    return _z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_DEFAULT);
}

int _z_write_ext(_z_session_t *zn, const _z_reskey_t reskey, const uint8_t *payload, const size_t len, const _z_encoding_t encoding, const uint8_t kind, const _z_congestion_control_t cong_ctrl)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    // Data info
    _z_data_info_t info;
    info.flags = 0;
    info.encoding = encoding;
    _Z_SET_FLAG(info.flags, _Z_DATA_INFO_ENC);
    info.kind = kind;
    _Z_SET_FLAG(info.flags, _Z_DATA_INFO_KIND);

    // Payload
    _z_payload_t pld;
    pld.len = len;
    pld.val = payload;

    // Congestion control
    int can_be_dropped = cong_ctrl == Z_CONGESTION_CONTROL_DROP;

    _z_zenoh_message_t z_msg = _z_msg_make_data(reskey, info, pld, can_be_dropped);

    return _z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, cong_ctrl);
}

/*------------------ Query ------------------*/
void _z_query(_z_session_t *zn, _z_reskey_t reskey, const _z_str_t predicate, const _z_query_target_t target, const _z_consolidation_strategy_t consolidation, _z_query_handler_t callback, void *arg)
{
    // Create the pending query object
    _z_pending_query_t *pq = (_z_pending_query_t *)malloc(sizeof(_z_pending_query_t));
    pq->id = _z_get_query_id(zn);
    pq->rname = _z_get_resource_name_from_key(zn, _Z_RESOURCE_IS_LOCAL, &reskey);
    pq->key = reskey;
    pq->predicate = _z_str_clone(predicate);
    pq->target = target;
    pq->consolidation = consolidation;
    pq->callback = callback;
    pq->pending_replies = NULL;
    pq->arg = arg;

    // Add the pending query to the current session
    _z_register_pending_query(zn, pq);

    _z_zenoh_message_t z_msg = _z_msg_make_query(reskey, pq->predicate, pq->id, pq->target, pq->consolidation);

    int res = _z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
    if (res != 0)
        _z_unregister_pending_query(zn, pq);
}

void reply_collect_handler(const _z_reply_t reply, const void *arg)
{
    _z_pending_query_collect_t *pqc = (_z_pending_query_collect_t *)arg;
    if (reply.tag == Z_REPLY_TAG_DATA)
    {
        _z_reply_data_t *rd = (_z_reply_data_t *)malloc(sizeof(_z_reply_data_t));
        rd->replier_kind = reply.data.replier_kind;
        _z_bytes_copy(&rd->replier_id, &reply.data.replier_id);
        rd->data.key = _z_reskey_duplicate(&reply.data.data.key);
        _z_bytes_copy(&rd->data.value, &reply.data.data.value);
        rd->data.encoding.prefix = reply.data.data.encoding.prefix;
        rd->data.encoding.suffix = reply.data.data.encoding.suffix ? _z_str_clone(reply.data.data.encoding.suffix) : NULL;

        pqc->replies = _z_reply_data_list_push(pqc->replies, rd);
    }
    else
    {
        // Signal that we have received all the replies
	_z_mutex_lock(&pqc->mutex); // Avoid condvar signal to be triggered before wait
        _z_condvar_signal(&pqc->cond_var);
	_z_mutex_unlock(&pqc->mutex);
    }
}

_z_reply_data_array_t _z_query_collect(_z_session_t *zn,
                                     _z_reskey_t reskey,
                                     const _z_str_t predicate,
                                     const _z_query_target_t target,
                                     const _z_consolidation_strategy_t consolidation)
{
    // Create the synchronization variables
    _z_pending_query_collect_t pqc;
    pqc.replies = NULL;
    _z_mutex_init(&pqc.mutex);
    _z_condvar_init(&pqc.cond_var);

    // Issue the query
    _z_mutex_lock(&pqc.mutex); // Get the lock on the query, released on condvar wait
    _z_query(zn, reskey, predicate, target, consolidation, reply_collect_handler, &pqc);
    _z_condvar_wait(&pqc.cond_var, &pqc.mutex); // Wait to be notified, releases pqc.mutex

    _z_reply_data_array_t rda;
    rda.len = _z_reply_data_list_len(pqc.replies);
    _z_reply_data_t *replies = (_z_reply_data_t *)malloc(rda.len * sizeof(_z_reply_data_t));
    for (unsigned int i = 0; i < rda.len; i++)
    {
        _z_reply_data_t *reply = _z_reply_data_list_head(pqc.replies);
        replies[i].replier_kind = reply->replier_kind;
        _z_bytes_move(&replies[i].replier_id, &reply->replier_id);
        replies[i].data.key = reply->data.key;
        _z_bytes_move(&replies[i].data.value, &reply->data.value);
        replies[i].data.encoding.prefix = reply->data.encoding.prefix;
        replies[i].data.encoding.suffix = reply->data.encoding.suffix ? _z_str_clone(reply->data.encoding.suffix) : NULL;

        pqc.replies = _z_reply_data_list_pop(pqc.replies);
    }
    rda.val = replies;

    _z_condvar_free(&pqc.cond_var);
    _z_mutex_free(&pqc.mutex);

    return rda;
}

/*------------------ Pull ------------------*/
void _z_pull(const z_subscriber_t *sub)
{
    _z_subscriber_t *s = _z_get_subscription_by_id(sub->zn, _Z_RESOURCE_IS_LOCAL, sub->id);
    if (s == NULL)
        return;

    _z_zint_t pull_id = _z_get_pull_id(sub->zn);
    _z_zint_t max_samples = 0; // @TODO: get the correct value for max_sample
    int is_final = 1;

    _z_zenoh_message_t z_msg = _z_msg_make_pull(s->key, pull_id, max_samples, is_final);

    if (_z_send_z_msg(sub->zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    // FIXME: Provide wrap for all allocated values, so that clear can be executed
    //        Still, currently it is not leaking, since owned by the subscription
    //_z_msg_clear(&z_msg);
}
