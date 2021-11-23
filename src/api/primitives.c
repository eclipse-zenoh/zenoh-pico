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
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/transport/link/tx.h"

zn_hello_array_t zn_scout(unsigned int what, zn_properties_t *config, unsigned long timeout)
{
    return _zn_scout(what, config, timeout, 0);
}

/*------------------ Resource Declaration ------------------*/
z_zint_t zn_declare_resource(zn_session_t *zn, zn_reskey_t reskey)
{
    _zn_resource_t *r = (_zn_resource_t *)malloc(sizeof(_zn_resource_t));
    r->id = _zn_get_resource_id(zn);
    r->key = reskey;

    int res = _zn_register_resource(zn, _ZN_IS_LOCAL, r);
    if (res != 0)
    {
        free(r);
        return ZN_RESOURCE_ID_NONE;
    }

    // Build the declare message to send on the wire
    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

    // We need to declare the resource
    unsigned int len = 1;
    z_msg.body.declare.declarations.len = len;
    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

    // Resource declaration
    z_msg.body.declare.declarations.val[0].header = _ZN_DECL_RESOURCE;
    z_msg.body.declare.declarations.val[0].body.res.id = r->id;
    z_msg.body.declare.declarations.val[0].body.res.key = _zn_reskey_clone(&r->key);
    if (r->key.rname)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // TODO: retransmission
    }

    _zn_zenoh_message_free(&z_msg);

    return r->id;
}

void zn_undeclare_resource(zn_session_t *zn, z_zint_t rid)
{
    _zn_resource_t *r = _zn_get_resource_by_id(zn, _ZN_IS_LOCAL, rid);
    if (r)
    {
        _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

        // We need to undeclare the resource and the publisher
        unsigned int len = 1;
        z_msg.body.declare.declarations.len = len;
        z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

        // Resource declaration
        z_msg.body.declare.declarations.val[0].header = _ZN_DECL_FORGET_RESOURCE;
        z_msg.body.declare.declarations.val[0].body.forget_res.rid = rid;

        if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
        {
            // TODO: retransmission
        }

        _zn_zenoh_message_free(&z_msg);

        _zn_unregister_resource(zn, _ZN_IS_LOCAL, r);
    }
}

/*------------------  Publisher Declaration ------------------*/
zn_publisher_t *zn_declare_publisher(zn_session_t *zn, zn_reskey_t reskey)
{
    zn_publisher_t *pub = (zn_publisher_t *)malloc(sizeof(zn_publisher_t));
    pub->zn = zn;
    pub->key = reskey;
    pub->id = _zn_get_entity_id(zn);

    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

    // We need to declare the resource and the publisher
    unsigned int len = 1;
    z_msg.body.declare.declarations.len = len;
    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

    // Publisher declaration
    z_msg.body.declare.declarations.val[0].header = _ZN_DECL_PUBLISHER;
    z_msg.body.declare.declarations.val[0].body.pub.key = _zn_reskey_clone(&reskey);
    ;
    // Mark the key as string if the key has resource name
    if (pub->key.rname)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // TODO: retransmission
    }

    _zn_zenoh_message_free(&z_msg);

    return pub;
}

void zn_undeclare_publisher(zn_publisher_t *pub)
{
    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

    // We need to undeclare the publisher
    unsigned int len = 1;
    z_msg.body.declare.declarations.len = len;
    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

    // Forget publisher declaration
    z_msg.body.declare.declarations.val[0].header = _ZN_DECL_FORGET_PUBLISHER;
    z_msg.body.declare.declarations.val[0].body.forget_pub.key = _zn_reskey_clone(&pub->key);
    // Mark the key as string if the key has resource name
    if (pub->key.rname)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);

    if (_zn_send_z_msg(pub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // TODO: retransmission
    }

    _zn_zenoh_message_free(&z_msg);

    free(pub);
}

/*------------------ Subscriber Declaration ------------------*/
zn_subscriber_t *zn_declare_subscriber(zn_session_t *zn, zn_reskey_t reskey, zn_subinfo_t sub_info, zn_data_handler_t callback, void *arg)
{
    _zn_subscriber_t *rs = (_zn_subscriber_t *)malloc(sizeof(_zn_subscriber_t));
    rs->id = _zn_get_entity_id(zn);
    rs->key = reskey;
    rs->info = sub_info;
    rs->callback = callback;
    rs->arg = arg;

    int res = _zn_register_subscription(zn, _ZN_IS_LOCAL, rs);
    if (res != 0)
    {
        free(rs);
        return NULL;
    }

    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

    // We need to declare the subscriber
    unsigned int len = 1;
    z_msg.body.declare.declarations.len = len;
    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

    // Subscriber declaration
    z_msg.body.declare.declarations.val[0].header = _ZN_DECL_SUBSCRIBER;
    if (reskey.rname)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);
    if (sub_info.mode != zn_submode_t_PUSH || sub_info.period)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_S);
    if (sub_info.reliability == zn_reliability_t_RELIABLE)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_R);

    z_msg.body.declare.declarations.val[0].body.sub.key = _zn_reskey_clone(&reskey);

    // SubMode
    z_msg.body.declare.declarations.val[0].body.sub.subinfo.mode = sub_info.mode;
    z_msg.body.declare.declarations.val[0].body.sub.subinfo.reliability = sub_info.reliability;
    z_msg.body.declare.declarations.val[0].body.sub.subinfo.period = sub_info.period;

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // TODO: retransmission
    }

    _zn_zenoh_message_free(&z_msg);

    zn_subscriber_t *subscriber = (zn_subscriber_t *)malloc(sizeof(zn_subscriber_t));
    subscriber->zn = zn;
    subscriber->id = rs->id;

    return subscriber;
}

void zn_undeclare_subscriber(zn_subscriber_t *sub)
{
    _zn_subscriber_t *s = _zn_get_subscription_by_id(sub->zn, _ZN_IS_LOCAL, sub->id);
    if (s)
    {
        _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

        // We need to undeclare the subscriber
        unsigned int len = 1;
        z_msg.body.declare.declarations.len = len;
        z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

        // Forget Subscriber declaration
        z_msg.body.declare.declarations.val[0].header = _ZN_DECL_FORGET_SUBSCRIBER;
        if (s->key.rname)
            _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);

        z_msg.body.declare.declarations.val[0].body.forget_sub.key = _zn_reskey_clone(&s->key);

        if (_zn_send_z_msg(sub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
        {
            // TODO: retransmission
        }

        _zn_zenoh_message_free(&z_msg);

        _zn_unregister_subscription(sub->zn, _ZN_IS_LOCAL, s);
    }

    free(sub);
}

/*------------------ Queryable Declaration ------------------*/
zn_queryable_t *zn_declare_queryable(zn_session_t *zn, zn_reskey_t reskey, unsigned int kind, zn_queryable_handler_t callback, void *arg)
{
    _zn_queryable_t *rq = (_zn_queryable_t *)malloc(sizeof(_zn_queryable_t));
    rq->id = _zn_get_entity_id(zn);
    rq->key = reskey;
    rq->kind = kind;
    rq->callback = callback;
    rq->arg = arg;

    int res = _zn_register_queryable(zn, rq);
    if (res != 0)
    {
        free(rq);
        return NULL;
    }

    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

    // We need to declare the queryable
    unsigned int len = 1;
    z_msg.body.declare.declarations.len = len;
    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

    // Queryable declaration
    z_msg.body.declare.declarations.val[0].header = _ZN_DECL_QUERYABLE;
    if (reskey.rname)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);
    if (kind != ZN_QUERYABLE_STORAGE)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_Q);

    z_msg.body.declare.declarations.val[0]
        .body.qle.key = _zn_reskey_clone(&reskey);
    z_msg.body.declare.declarations.val[0]
        .body.qle.kind = (z_zint_t)kind;

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // TODO: retransmission
    }

    _zn_zenoh_message_free(&z_msg);

    zn_queryable_t *queryable = (zn_queryable_t *)malloc(sizeof(zn_queryable_t));
    queryable->zn = zn;
    queryable->id = rq->id;

    return queryable;
}

void zn_undeclare_queryable(zn_queryable_t *qle)
{
    _zn_queryable_t *q = _zn_get_queryable_by_id(qle->zn, qle->id);
    if (q)
    {
        _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

        // We need to undeclare the subscriber
        unsigned int len = 1;
        z_msg.body.declare.declarations.len = len;
        z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

        // Forget Subscriber declaration
        z_msg.body.declare.declarations.val[0].header = _ZN_DECL_FORGET_QUERYABLE;
        if (q->key.rname)
            _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);

        z_msg.body.declare.declarations.val[0].body.forget_sub.key = _zn_reskey_clone(&q->key);

        if (_zn_send_z_msg(qle->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
        {
            // TODO: retransmission
        }

        _zn_zenoh_message_free(&z_msg);

        _zn_unregister_queryable(qle->zn, q);
    }

    free(qle);
}

void zn_send_reply(zn_query_t *query, const z_str_t key, const uint8_t *payload, size_t len)
{
    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DATA);

    // Build the reply context decorator. This is NOT the final reply.
    z_msg.reply_context = _zn_reply_context_init();
    z_msg.reply_context->qid = query->qid;
    z_msg.reply_context->replier_kind = query->kind;
    z_msg.reply_context->replier_id = query->zn->tp_manager->local_pid;

    // Build the data payload
    z_msg.body.data.payload.val = payload;
    z_msg.body.data.payload.len = len;
    // @TODO: use numerical resources if possible
    z_msg.body.data.key.rid = ZN_RESOURCE_ID_NONE;
    z_msg.body.data.key.rname = (z_str_t)key;
    if (z_msg.body.data.key.rname)
        _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_K);
    // Do not set any data_info

    if (_zn_send_z_msg(query->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        // TODO: retransmission
    }

    free(z_msg.reply_context);
}


/*------------------ Write ------------------*/
int zn_write(zn_session_t *zn, zn_reskey_t reskey, const uint8_t *payload, size_t length)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DATA);
    // Eventually mark the message for congestion control
    if (ZN_CONGESTION_CONTROL_DEFAULT == zn_congestion_control_t_DROP)
        _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_D);
    // Set the resource key
    z_msg.body.data.key = reskey;
    _ZN_SET_FLAG(z_msg.header, reskey.rname ? _ZN_FLAG_Z_K : 0);

    // Set the payload
    z_msg.body.data.payload.len = length;
    z_msg.body.data.payload.val = (uint8_t *)payload;

    return _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, ZN_CONGESTION_CONTROL_DEFAULT);
}

int zn_write_ext(zn_session_t *zn, zn_reskey_t reskey, const uint8_t *payload, size_t length, uint8_t encoding, uint8_t kind, zn_congestion_control_t cong_ctrl)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DATA);
    // Eventually mark the message for congestion control
    if (cong_ctrl == zn_congestion_control_t_DROP)
        _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_D);
    // Set the resource key
    z_msg.body.data.key = reskey;
    _ZN_SET_FLAG(z_msg.header, reskey.rname ? _ZN_FLAG_Z_K : 0);

    // Set the data info
    _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_I);
    _zn_data_info_t info;
    info.flags = 0;
    info.encoding = encoding;
    _ZN_SET_FLAG(info.flags, _ZN_DATA_INFO_ENC);
    info.kind = kind;
    _ZN_SET_FLAG(info.flags, _ZN_DATA_INFO_KIND);
    z_msg.body.data.info = info;

    // Set the payload
    z_msg.body.data.payload.len = length;
    z_msg.body.data.payload.val = (uint8_t *)payload;

    return _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, cong_ctrl);
}

/*------------------ Query ------------------*/
void zn_query(zn_session_t *zn, zn_reskey_t reskey, const z_str_t predicate, zn_query_target_t target, zn_query_consolidation_t consolidation, zn_query_handler_t callback, void *arg)
{
    // Create the pending query object
    _zn_pending_query_t *pq = (_zn_pending_query_t *)malloc(sizeof(_zn_pending_query_t));
    pq->id = _zn_get_query_id(zn);
    pq->key = reskey;
    pq->predicate = strdup(predicate);
    pq->target = target;
    pq->consolidation = consolidation;
    pq->callback = callback;
    pq->pending_replies = z_list_empty;
    pq->arg = arg;

    // Add the pending query to the current session
    _zn_register_pending_query(zn, pq);

    // Send the query
    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_QUERY);
    z_msg.body.query.qid = pq->id;
    z_msg.body.query.key = reskey;
    _ZN_SET_FLAG(z_msg.header, reskey.rname ? _ZN_FLAG_Z_K : 0);
    z_msg.body.query.predicate = (z_str_t)predicate;

    zn_query_target_t qtd = zn_query_target_default();
    if (!zn_query_target_equal(&target, &qtd))
    {
        _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_T);
        z_msg.body.query.target = target;
    }

    z_msg.body.query.consolidation = consolidation;

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

        z_vec_append(&pqc->replies, rd);
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
                                       zn_query_target_t target,
                                       zn_query_consolidation_t consolidation)
{
    // Create the synchronization variables
    _zn_pending_query_collect_t pqc;
    z_mutex_init(&pqc.mutex);
    z_condvar_init(&pqc.cond_var);
    pqc.replies = z_vec_make(1);

    // Issue the query
    zn_query(zn, reskey, predicate, target, consolidation, reply_collect_handler, &pqc);

    // Wait to be notified
    z_mutex_lock(&pqc.mutex);
    z_condvar_wait(&pqc.cond_var, &pqc.mutex);

    zn_reply_data_array_t rda;
    rda.len = z_vec_len(&pqc.replies);
    zn_reply_data_t *replies = (zn_reply_data_t *)malloc(rda.len * sizeof(zn_reply_data_t));
    for (unsigned int i = 0; i < rda.len; i++)
    {
        zn_reply_data_t *reply = (zn_reply_data_t *)z_vec_get(&pqc.replies, i);
        replies[i].replier_kind = reply->replier_kind;
        _z_bytes_move(&replies[i].replier_id, &reply->replier_id);
        _z_string_move(&replies[i].data.key, &reply->data.key);
        _z_bytes_move(&replies[i].data.value, &reply->data.value);
    }
    rda.val = replies;

    z_vec_free(&pqc.replies);
    z_condvar_free(&pqc.cond_var);
    z_mutex_free(&pqc.mutex);

    return rda;
}

/*------------------ Pull ------------------*/
int zn_pull(zn_subscriber_t *sub)
{
    _zn_subscriber_t *s = _zn_get_subscription_by_id(sub->zn, _ZN_IS_LOCAL, sub->id);
    if (s)
    {
        _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_PULL);
        z_msg.body.pull.key = _zn_reskey_clone(&s->key);
        _ZN_SET_FLAG(z_msg.header, s->key.rname ? _ZN_FLAG_Z_K : 0);
        _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_F);

        z_msg.body.pull.pull_id = _zn_get_pull_id(sub->zn);
        // @TODO: get the correct value for max_sample
        z_msg.body.pull.max_samples = 0;
        // _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_N);

        if (_zn_send_z_msg(sub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
        {
            // TODO: retransmission
        }

        _zn_zenoh_message_free(&z_msg);

        return 0;
    }
    else
    {
        return -1;
    }
}
