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
#include "zenoh-pico/net/memory.h"
#include "zenoh-pico/net/logger.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/protocol/utils.h"

_z_hello_array_t _z_scout(const _z_zint_t what, const _z_config_t *config, const uint32_t timeout)
{
    return _z_scout_inner(what, config, timeout, 0);
}

/*------------------ Resource Declaration ------------------*/
_z_zint_t _z_declare_resource(_z_session_t *zn, _z_keyexpr_t keyexpr)
{
    _z_resource_t *r = (_z_resource_t *)malloc(sizeof(_z_resource_t));
    r->_id = _z_get_resource_id(zn);
    r->_key = keyexpr;

    // FIXME: remove when resource declaration is implemented for multicast transport
    if (zn->_tp->_type == _Z_TRANSPORT_MULTICAST_TYPE)
        goto ERR;

    int res = _z_register_resource(zn, _Z_RESOURCE_IS_LOCAL, r);
    if (res != 0)
        goto ERR;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_resource(r->_id, _z_keyexpr_duplicate(&r->_key));

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);

    return r->_id;

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
    declarations._val[0] = _z_msg_make_declaration_forget_resource(rid);

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
_z_publisher_t *_z_declare_publisher(_z_session_t *zn, _z_keyexpr_t keyexpr, int8_t local_routing, z_congestion_control_t congestion_control, z_priority_t priority)
{
    _z_publisher_t *pub = (_z_publisher_t *)malloc(sizeof(_z_publisher_t));
    pub->_zn = zn;
    pub->_key = keyexpr;
    pub->_id = _z_get_entity_id(zn);
    pub->_local_routing = local_routing;
    pub->_congestion_control = congestion_control;
    pub->_priority = priority;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_publisher(_z_keyexpr_duplicate(&keyexpr));

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);

    return pub;
}

void _z_undeclare_publisher(_z_publisher_t *pub)
{
    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_forget_publisher(_z_keyexpr_duplicate(&pub->_key));

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(pub->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);
}

/*------------------ Subscriber Declaration ------------------*/
_z_subscriber_t *_z_declare_subscriber(_z_session_t *zn, _z_keyexpr_t keyexpr, _z_subinfo_t sub_info, _z_data_handler_t callback, void *arg)
{
    _z_subscription_t *rs = (_z_subscription_t *)malloc(sizeof(_z_subscription_t));
    rs->_id = _z_get_entity_id(zn);
    rs->_key = _z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_LOCAL, &keyexpr);
    rs->_info = sub_info;
    rs->_callback = callback;
    rs->_arg = arg;

    int res = _z_register_subscription(zn, _Z_RESOURCE_IS_LOCAL, rs);
    if (res != 0)
        goto ERR;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_subscriber(keyexpr, sub_info);

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);

    _z_subscriber_t *subscriber = (_z_subscriber_t *)malloc(sizeof(_z_subscriber_t));
    subscriber->_zn = zn;
    subscriber->_id = rs->_id;

    return subscriber;

ERR:
    _z_keyexpr_clear(&rs->_key);
    free(rs);
    return NULL;
}

void _z_undeclare_subscriber(_z_subscriber_t *sub)
{
    _z_subscription_t *s = _z_get_subscription_by_id(sub->_zn, _Z_RESOURCE_IS_LOCAL, sub->_id);
    if (s == NULL)
        return;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    _z_keyexpr_t key = _z_keyexpr_duplicate(&s->_key);
    declarations._val[0] = _z_msg_make_declaration_forget_subscriber(key);

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(sub->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);
    _z_unregister_subscription(sub->_zn, _Z_RESOURCE_IS_LOCAL, s);
}

/*------------------ Queryable Declaration ------------------*/
_z_queryable_t *_z_declare_queryable(_z_session_t *zn, _z_keyexpr_t keyexpr, uint8_t complete, _z_questionable_handler_t callback, void *arg)
{
    _z_questionable_t *rq = (_z_questionable_t *)malloc(sizeof(_z_questionable_t));
    rq->_id = _z_get_entity_id(zn);
    rq->_key = _z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_LOCAL, &keyexpr);
    rq->_complete = complete;
    rq->_kind = Z_QUERYABLE_EVAL;
    rq->_callback = callback;
    rq->_arg = arg;

    int res = _z_register_queryable(zn, rq);
    if (res != 0)
        goto ERR;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    declarations._val[0] = _z_msg_make_declaration_queryable(_z_keyexpr_duplicate(&keyexpr), rq->_kind, rq->_complete, _Z_QUERYABLE_DISTANCE_DEFAULT);

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);

    _z_queryable_t *queryable = (_z_queryable_t *)malloc(sizeof(_z_queryable_t));
    queryable->_zn = zn;
    queryable->_id = rq->_id;

    return queryable;

ERR:
    _z_keyexpr_clear(&rq->_key);
    free(rq);
    return NULL;
}

void _z_undeclare_queryable(_z_queryable_t *qle)
{
    _z_questionable_t *q = _z_get_queryable_by_id(qle->_zn, qle->_id);
    if (q == NULL)
        return;

    _z_declaration_array_t declarations = _z_declaration_array_make(1);
    _z_keyexpr_t key = _z_keyexpr_duplicate(&q->_key);
    declarations._val[0] = _z_msg_make_declaration_forget_queryable(key, q->_kind);

    // Build the declare message to send on the wire
    _z_zenoh_message_t z_msg = _z_msg_make_declare(declarations);

    if (_z_send_z_msg(qle->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    _z_msg_clear(&z_msg);

    _z_unregister_queryable(qle->_zn, q);
}

void _z_send_reply(const z_query_t *query, const char *key, const uint8_t *payload, const size_t len)
{
    // Build the reply context decorator. This is NOT the final reply._
    _z_bytes_t pid = _z_bytes_wrap(((_z_session_t*)query->_zn)->_tp_manager->_local_pid.start, ((_z_session_t*)query->_zn)->_tp_manager->_local_pid.len);
    _z_reply_context_t *rctx = _z_msg_make_reply_context(query->_qid, pid, query->_kind, 0);

    // @TODO: use numerical resources if possible
    // ResKey
    _z_keyexpr_t keyexpr;
    keyexpr.id = Z_RESOURCE_ID_NONE;
    keyexpr.suffix = key;

    // Empty data info
    _z_data_info_t di;
    di._flags = 0;

    // Payload
    _z_payload_t pld;
    pld.len = len;
    pld.start = payload;

    // Congestion control
    int can_be_dropped = 0;

    _z_zenoh_message_t z_msg = _z_msg_make_reply(keyexpr, di, pld, can_be_dropped, rctx);

    if (_z_send_z_msg(query->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    free(rctx);
    // FIXME: Provide wrap for all allocated values, so that clear can be executed
    //        Still, currently it is not leaking, since all values are owned by the user
    //_z_msg_clear(&z_msg);
}

/*------------------ Write ------------------*/
int _z_write(_z_session_t *zn, const _z_keyexpr_t keyexpr, const uint8_t *payload, const size_t len)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    // Empty data info
    _z_data_info_t info;
    info._flags = 0;

    // Payload
    _z_payload_t pld;
    pld.len = len;
    pld.start = payload;

    // Congestion control
    int can_be_dropped = Z_CONGESTION_CONTROL_DEFAULT == Z_CONGESTION_CONTROL_DROP;

    _z_zenoh_message_t z_msg = _z_msg_make_data(keyexpr, info, pld, can_be_dropped);

    return _z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_DEFAULT);
}

int _z_write_ext(_z_session_t *zn, const _z_keyexpr_t keyexpr, const uint8_t *payload, const size_t len, const _z_encoding_t encoding, const uint8_t kind, const z_congestion_control_t cong_ctrl)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    // Data info
    _z_data_info_t info;
    info._flags = 0;
    info._encoding = encoding;
    _Z_SET_FLAG(info._flags, _Z_DATA_INFO_ENC);
    info._kind = kind;
    _Z_SET_FLAG(info._flags, _Z_DATA_INFO_KIND);

    // Payload
    _z_payload_t pld;
    pld.len = len;
    pld.start = payload;

    // Congestion control
    int can_be_dropped = cong_ctrl == Z_CONGESTION_CONTROL_DROP;

    _z_zenoh_message_t z_msg = _z_msg_make_data(keyexpr, info, pld, can_be_dropped);

    return _z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, cong_ctrl);
}

/*------------------ Query ------------------*/
void _z_query(_z_session_t *zn, _z_keyexpr_t keyexpr, const char *predicate, const _z_target_t target, const _z_consolidation_strategy_t consolidation, _z_query_handler_t callback, void *arg)
{
    // Create the pending query object
    _z_pending_query_t *pq = (_z_pending_query_t *)malloc(sizeof(_z_pending_query_t));
    pq->_id = _z_get_query_id(zn);
    pq->_key = _z_get_expanded_key_from_key(zn, _Z_RESOURCE_IS_LOCAL, &keyexpr);
    pq->_predicate = _z_str_clone(predicate);
    pq->_target = target;
    pq->_consolidation = consolidation;
    pq->_callback = callback;
    pq->_pending_replies = NULL;
    pq->_arg = arg;

    // Add the pending query to the current session
    _z_register_pending_query(zn, pq);

    _z_zenoh_message_t z_msg = _z_msg_make_query(keyexpr, pq->_predicate, pq->_id, pq->_target, pq->_consolidation);

    int res = _z_send_z_msg(zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK);
    if (res != 0)
        _z_unregister_pending_query(zn, pq);
}

void _z_reply_collect_handler(const _z_reply_t *reply, const void *arg)
{
    _z_pending_query_collect_t *pqc = (_z_pending_query_collect_t *)arg;
    if (reply->tag == Z_REPLY_TAG_DATA)
    {
        _z_reply_data_t *rd = (_z_reply_data_t *)malloc(sizeof(_z_reply_data_t));
        rd->replier_kind = reply->data.replier_kind;
        _z_bytes_copy(&rd->replier_id, &reply->data.replier_id);
        rd->sample.key = _z_keyexpr_duplicate(&reply->data.sample.key);
        _z_bytes_copy(&rd->sample.value, &reply->data.sample.value);
        rd->sample.encoding.prefix = reply->data.sample.encoding.prefix;
        rd->sample.encoding.suffix = reply->data.sample.encoding.suffix ? _z_str_clone(reply->data.sample.encoding.suffix) : _z_str_clone("");
        rd->sample.kind = reply->data.sample.kind;
        rd->sample.timestamp = _z_timestamp_duplicate(&reply->data.sample.timestamp);

        pqc->_replies = _z_reply_data_list_push(pqc->_replies, rd);
    }
    else
    {
        // Signal that we have received all the replies
        _z_mutex_lock(&pqc->_mutex); // Avoid condvar signal to be triggered before wait
        _z_condvar_signal(&pqc->_cond_var);
        _z_mutex_unlock(&pqc->_mutex);
    }
}

_z_reply_data_array_t _z_query_collect(_z_session_t *zn,
                                     _z_keyexpr_t keyexpr,
                                     const char *predicate,
                                     const _z_target_t target,
                                     const _z_consolidation_strategy_t consolidation)
{
    // Create the synchronization variables
    _z_pending_query_collect_t pqc;
    pqc._replies = NULL;
    _z_mutex_init(&pqc._mutex);
    _z_condvar_init(&pqc._cond_var);

    // Issue the query
    _z_mutex_lock(&pqc._mutex); // Get the lock on the query, released on condvar wait
    _z_query(zn, keyexpr, predicate, target, consolidation, _z_reply_collect_handler, &pqc);
    _z_condvar_wait(&pqc._cond_var, &pqc._mutex); // Wait to be notified, releases pqc._mutex

    _z_reply_data_array_t rda;
    rda._len = _z_reply_data_list_len(pqc._replies);
    _z_reply_data_t *replies = (_z_reply_data_t *)malloc(rda._len * sizeof(_z_reply_data_t));
    for (unsigned int i = 0; i < rda._len; i++)
    {
        _z_reply_data_t *reply = _z_reply_data_list_head(pqc._replies);
        replies[i].replier_kind = reply->replier_kind;
        _z_bytes_move(&replies[i].replier_id, &reply->replier_id);
        _z_sample_move(&replies[i].sample, &reply->sample);

        pqc._replies = _z_reply_data_list_pop(pqc._replies);
    }
    rda._val = replies;

    _z_condvar_free(&pqc._cond_var);
    _z_mutex_free(&pqc._mutex);

    return rda;
}

/*------------------ Pull ------------------*/
void _z_pull(const _z_subscriber_t *sub)
{
    _z_subscription_t *s = _z_get_subscription_by_id(sub->_zn, _Z_RESOURCE_IS_LOCAL, sub->_id);
    if (s == NULL)
        return;

    _z_zint_t pull_id = _z_get_pull_id(sub->_zn);
    _z_zint_t max_samples = 0; // @TODO: get the correct value for max_sample
    int is_final = 1;

    _z_zenoh_message_t z_msg = _z_msg_make_pull(s->_key, pull_id, max_samples, is_final);

    if (_z_send_z_msg(sub->_zn, &z_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK) != 0)
    {
        // @TODO: retransmission
    }

    // FIXME: Provide wrap for all allocated values, so that clear can be executed
    //        Still, currently it is not leaking, since owned by the subscription
    //_z_msg_clear(&z_msg);
}
