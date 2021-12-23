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

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/query.h"
#include "zenoh-pico/api/logger.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/transport/link/tx.h"

zn_hello_array_t zn_scout(const unsigned int what, const zn_properties_t *config, const unsigned long timeout)
{
    return _zn_scout(what, config, timeout, 0);
}

/*------------------ Resource Declaration ------------------*/
z_zint_t zn_declare_resource(zn_session_t *zn, zn_reskey_t reskey)
{
    _zn_resource_t *r = (_zn_resource_t *)malloc(sizeof(_zn_resource_t));
    r->id = _zn_get_resource_id(zn);
    r->key = reskey;

    // FIXME: remove when resource declaration is implemented for multicast transport
    if (zn->tp->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        goto ERR;

    int res = _zn_register_resource(zn, _ZN_IS_LOCAL, r);
    if (res != 0)
        goto ERR;

    _zn_declaration_array_t declarations = _zn_declaration_array_make(1);
    declarations.val[0] = _zn_z_msg_make_declaration_resource(r->id, _zn_reskey_duplicate(&r->key));

    // Build the declare message to send on the wire
    _zn_zenoh_message_t z_msg = _zn_z_msg_make_declare(declarations);

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _zn_z_msg_clear(&z_msg);

    return r->id;

ERR:
    free(r);
    return ZN_RESOURCE_ID_NONE;
}

void zn_undeclare_resource(zn_session_t *zn, const z_zint_t rid)
{
    _zn_resource_t *r = _zn_get_resource_by_id(zn, _ZN_IS_LOCAL, rid);
    if (r == NULL)
        return;

    _zn_declaration_array_t declarations = _zn_declaration_array_make(1);
    declarations.val[0] = _zn_z_msg_make_declaration_forget_resource(rid);

    // Build the declare message to send on the wire
    _zn_zenoh_message_t z_msg = _zn_z_msg_make_declare(declarations);

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _zn_z_msg_clear(&z_msg);
    _zn_unregister_resource(zn, _ZN_IS_LOCAL, r);
}

/*------------------  Publisher Declaration ------------------*/
zn_publisher_t *zn_declare_publisher(zn_session_t *zn, zn_reskey_t reskey)
{
    zn_publisher_t *pub = (zn_publisher_t *)malloc(sizeof(zn_publisher_t));
    pub->zn = zn;
    pub->key = reskey;
    pub->id = _zn_get_entity_id(zn);

    _zn_declaration_array_t declarations = _zn_declaration_array_make(1);
    declarations.val[0] = _zn_z_msg_make_declaration_publisher(_zn_reskey_duplicate(&reskey));

    // Build the declare message to send on the wire
    _zn_zenoh_message_t z_msg = _zn_z_msg_make_declare(declarations);

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _zn_z_msg_clear(&z_msg);

    return pub;
}

void zn_undeclare_publisher(zn_publisher_t *pub)
{
    _zn_declaration_array_t declarations = _zn_declaration_array_make(1);
    declarations.val[0] = _zn_z_msg_make_declaration_forget_publisher(_zn_reskey_duplicate(&pub->key));

    // Build the declare message to send on the wire
    _zn_zenoh_message_t z_msg = _zn_z_msg_make_declare(declarations);

    if (_zn_send_z_msg(pub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _zn_z_msg_clear(&z_msg);
}

/*------------------ Subscriber Declaration ------------------*/
zn_subscriber_t *zn_declare_subscriber(zn_session_t *zn, zn_reskey_t reskey, zn_subinfo_t sub_info, zn_data_handler_t callback, void *arg)
{
    _zn_subscriber_t *rs = (_zn_subscriber_t *)malloc(sizeof(_zn_subscriber_t));
    rs->id = _zn_get_entity_id(zn);
    rs->rname = _zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &reskey);
    rs->key = reskey;
    rs->info = sub_info;
    rs->callback = callback;
    rs->arg = arg;

    int res = _zn_register_subscription(zn, _ZN_IS_LOCAL, rs);
    if (res != 0)
        goto ERR;

    _zn_declaration_array_t declarations = _zn_declaration_array_make(1);
    declarations.val[0] = _zn_z_msg_make_declaration_subscriber(_zn_reskey_duplicate(&reskey), sub_info);

    // Build the declare message to send on the wire
    _zn_zenoh_message_t z_msg = _zn_z_msg_make_declare(declarations);

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _zn_z_msg_clear(&z_msg);

    zn_subscriber_t *subscriber = (zn_subscriber_t *)malloc(sizeof(zn_subscriber_t));
    subscriber->zn = zn;
    subscriber->id = rs->id;

    return subscriber;

ERR:
    _z_str_clear(rs->rname);
    free(rs);
    return NULL;
}

void zn_undeclare_subscriber(zn_subscriber_t *sub)
{
    _zn_subscriber_t *s = _zn_get_subscription_by_id(sub->zn, _ZN_IS_LOCAL, sub->id);
    if (s == NULL)
        return;

    _zn_declaration_array_t declarations = _zn_declaration_array_make(1);
    zn_reskey_t key;
    key.rid = ZN_RESOURCE_ID_NONE;
    key.rname = _z_str_clone(s->rname);
    declarations.val[0] = _zn_z_msg_make_declaration_forget_subscriber(key);

    // Build the declare message to send on the wire
    _zn_zenoh_message_t z_msg = _zn_z_msg_make_declare(declarations);

    if (_zn_send_z_msg(sub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _zn_z_msg_clear(&z_msg);
    _zn_unregister_subscription(sub->zn, _ZN_IS_LOCAL, s);
}

/*------------------ Queryable Declaration ------------------*/
zn_queryable_t *zn_declare_queryable(zn_session_t *zn, zn_reskey_t reskey, unsigned int kind, zn_queryable_handler_t callback, void *arg)
{
    _zn_queryable_t *rq = (_zn_queryable_t *)malloc(sizeof(_zn_queryable_t));
    rq->id = _zn_get_entity_id(zn);
    rq->rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &reskey);
    rq->kind = kind;
    rq->callback = callback;
    rq->arg = arg;

    int res = _zn_register_queryable(zn, rq);
    if (res != 0)
        goto ERR;

    _zn_declaration_array_t declarations = _zn_declaration_array_make(1);
    declarations.val[0] = _zn_z_msg_make_declaration_queryable(_zn_reskey_duplicate(&reskey), kind, _ZN_QUERYABLE_COMPLETE_DEFAULT, _ZN_QUERYABLE_DISTANCE_DEFAULT);

    // Build the declare message to send on the wire
    _zn_zenoh_message_t z_msg = _zn_z_msg_make_declare(declarations);

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _zn_z_msg_clear(&z_msg);

    zn_queryable_t *queryable = (zn_queryable_t *)malloc(sizeof(zn_queryable_t));
    queryable->zn = zn;
    queryable->id = rq->id;

    return queryable;

ERR:
    _z_str_clear(rq->rname);
    free(rq);
    return NULL;
}

void zn_undeclare_queryable(zn_queryable_t *qle)
{
    _zn_queryable_t *q = _zn_get_queryable_by_id(qle->zn, qle->id);
    if (q == NULL)
        return;

    _zn_declaration_array_t declarations = _zn_declaration_array_make(1);
    zn_reskey_t key;
    key.rid = ZN_RESOURCE_ID_NONE;
    key.rname = _z_str_clone(q->rname);
    declarations.val[0] = _zn_z_msg_make_declaration_forget_queryable(key);

    // Build the declare message to send on the wire
    _zn_zenoh_message_t z_msg = _zn_z_msg_make_declare(declarations);

    if (_zn_send_z_msg(qle->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _zn_z_msg_clear(&z_msg);

    _zn_unregister_queryable(qle->zn, q);
}

void zn_send_reply(zn_query_t *query, z_str_t key, const uint8_t *payload, const size_t len)
{
    // Build the reply context decorator. This is NOT the final reply.
    z_bytes_t pid = _z_bytes_wrap(((zn_session_t*)query->zn)->tp_manager->local_pid.val, ((zn_session_t*)query->zn)->tp_manager->local_pid.len);

    _zn_reply_context_t *rctx = _zn_z_msg_make_reply_context(query->qid, pid, query->kind, 0);

    // @TODO: use numerical resources if possible
    // ResKey
    zn_reskey_t reskey;
    reskey.rid = ZN_RESOURCE_ID_NONE;
    reskey.rname = (z_str_t)key;

    // Empty data info
    _zn_data_info_t di;
    di.flags = 0;

    // Payload
    _zn_payload_t pld;
    pld.len = len;
    pld.val = payload;

    // Congestion control
    int can_be_dropped = 0;

    _zn_zenoh_message_t z_msg = _zn_z_msg_make_reply(reskey, di, pld, can_be_dropped, rctx);

    if (_zn_send_z_msg(query->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    free(rctx);
}

/*------------------ Write ------------------*/
int zn_write(zn_session_t *zn, const zn_reskey_t reskey, const uint8_t *payload, const size_t len)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    // Empty data info
    _zn_data_info_t info;
    info.flags = 0;

    // Payload
    _zn_payload_t pld;
    pld.len = len;
    pld.val = payload;

    // Congestion control
    int can_be_dropped = ZN_CONGESTION_CONTROL_DEFAULT == zn_congestion_control_t_DROP;

    _zn_zenoh_message_t z_msg = _zn_z_msg_make_data(reskey, info, pld, can_be_dropped);

    return _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, ZN_CONGESTION_CONTROL_DEFAULT);
}

int zn_write_ext(zn_session_t *zn, const zn_reskey_t reskey, const uint8_t *payload, const size_t len, uint8_t encoding, const uint8_t kind, const zn_congestion_control_t cong_ctrl)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    // Data info
    _zn_data_info_t info;
    info.flags = 0;
    info.encoding.prefix = encoding;
    info.encoding.suffix = ""; // @TODO: empty for now, but expose this in the function signature
    _ZN_SET_FLAG(info.flags, _ZN_DATA_INFO_ENC);
    info.kind = kind;
    _ZN_SET_FLAG(info.flags, _ZN_DATA_INFO_KIND);

    // Payload
    _zn_payload_t pld;
    pld.len = len;
    pld.val = payload;

    // Congestion control
    int can_be_dropped = cong_ctrl == zn_congestion_control_t_DROP;

    _zn_zenoh_message_t z_msg = _zn_z_msg_make_data(reskey, info, pld, can_be_dropped);

    return _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, cong_ctrl);
}

/*------------------ Query ------------------*/
void zn_query(zn_session_t *zn, zn_reskey_t reskey, const z_str_t predicate, const zn_query_target_t target, const zn_query_consolidation_t consolidation, zn_query_handler_t callback, void *arg)
{
    // Create the pending query object
    _zn_pending_query_t *pq = (_zn_pending_query_t *)malloc(sizeof(_zn_pending_query_t));
    pq->id = _zn_get_query_id(zn);
    pq->key = reskey;
    pq->predicate = _z_str_clone(predicate);
    pq->target = target;
    pq->consolidation = consolidation;
    pq->callback = callback;
    pq->pending_replies = NULL;
    pq->arg = arg;

    // Add the pending query to the current session
    _zn_register_pending_query(zn, pq);

    _zn_zenoh_message_t z_msg = _zn_z_msg_make_query(pq->key, pq->predicate, pq->id, pq->target, pq->consolidation);

    int res = _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
    if (res != 0)
        _zn_unregister_pending_query(zn, pq);
}

void reply_collect_handler(const zn_reply_t reply, const void *arg)
{
    _zn_pending_query_collect_t *pqc = (_zn_pending_query_collect_t *)arg;
    if (reply.tag == zn_reply_t_Tag_DATA)
    {
        zn_reply_data_t *rd = (zn_reply_data_t *)malloc(sizeof(zn_reply_data_t));
        rd->replier_kind = reply.data.replier_kind;
        _z_bytes_copy(&rd->replier_id, &reply.data.replier_id);
        _z_string_copy(&rd->data.key, &reply.data.data.key);
        _z_bytes_copy(&rd->data.value, &reply.data.data.value);

        pqc->replies = _zn_reply_data_list_push(pqc->replies, rd);
    }
    else
    {
        // Signal that we have received all the replies
        z_condvar_signal(&pqc->cond_var);
    }
}

zn_reply_data_array_t zn_query_collect(zn_session_t *zn,
                                       zn_reskey_t reskey,
                                       const z_str_t predicate,
                                       const zn_query_target_t target,
                                       const zn_query_consolidation_t consolidation)
{
    // Create the synchronization variables
    _zn_pending_query_collect_t pqc;
    z_mutex_init(&pqc.mutex);
    z_condvar_init(&pqc.cond_var);
    pqc.replies = NULL;

    // Issue the query
    zn_query(zn, reskey, predicate, target, consolidation, reply_collect_handler, &pqc);

    // Wait to be notified
    z_mutex_lock(&pqc.mutex);
    z_condvar_wait(&pqc.cond_var, &pqc.mutex);

    zn_reply_data_array_t rda;
    rda.len = _zn_reply_data_list_len(pqc.replies);
    zn_reply_data_t *replies = (zn_reply_data_t *)malloc(rda.len * sizeof(zn_reply_data_t));
    for (unsigned int i = 0; i < rda.len; i++)
    {
        zn_reply_data_t *reply = _zn_reply_data_list_head(pqc.replies);

        replies[i].replier_kind = reply->replier_kind;
        _z_bytes_move(&replies[i].replier_id, &reply->replier_id);
        _z_string_move(&replies[i].data.key, &reply->data.key);
        _z_bytes_move(&replies[i].data.value, &reply->data.value);

        pqc.replies = _zn_reply_data_list_pop(pqc.replies);
    }
    rda.val = replies;

    z_condvar_free(&pqc.cond_var);
    z_mutex_free(&pqc.mutex);

    return rda;
}

/*------------------ Pull ------------------*/
int zn_pull(const zn_subscriber_t *sub)
{
    _zn_subscriber_t *s = _zn_get_subscription_by_id(sub->zn, _ZN_IS_LOCAL, sub->id);
    if (s == NULL)
        return -1;

    z_zint_t pull_id = _zn_get_pull_id(sub->zn);
    z_zint_t max_samples = 0; // @TODO: get the correct value for max_sample
    int is_final = 1;

    _zn_zenoh_message_t z_msg = _zn_z_msg_make_pull(s->key, pull_id, max_samples, is_final);

    if (_zn_send_z_msg(sub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _zn_z_msg_clear(&z_msg);

    return 0;
}
