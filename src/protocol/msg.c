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

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/utils/logging.h"

/*=============================*/
/*        Message fields       */
/*=============================*/
/*------------------ Payload field ------------------*/
void _z_payload_clear(_z_payload_t *p)
{
    _z_bytes_clear(p);
}

/*------------------ Timestamp Field ------------------*/
void _z_timestamp_clear(_z_timestamp_t *ts)
{
    _z_bytes_clear(&ts->id);
}

/*------------------ ResKey Field ------------------*/
void _z_reskey_clear(_z_reskey_t *rk)
{
    rk->rid = 0;
    if (rk->rname != NULL)
        _z_str_clear(rk->rname);
}

void _z_reskey_free(_z_reskey_t **rk)
{
    _z_reskey_t *ptr = (_z_reskey_t *)*rk;
    _z_reskey_clear(ptr);

    free(ptr);
    *rk = NULL;
}

/*------------------ Locators Field ------------------*/
void _z_locators_clear(_z_locator_array_t *ls)
{
    _z_locator_array_clear(ls);
}

/*=============================*/
/*      Message decorators     */
/*=============================*/
/*------------------ Attachment Decorator ------------------*/
void _z_t_msg_clear_attachment(_z_attachment_t *a)
{
    _z_payload_clear(&a->payload);
}

/*------------------ ReplyContext Decorator ------------------*/
_z_reply_context_t *_z_msg_make_reply_context(_z_zint_t qid, _z_bytes_t replier_id, _z_zint_t replier_kind, int is_final)
{
    _z_reply_context_t *rctx = (_z_reply_context_t *)malloc(sizeof(_z_reply_context_t));

    rctx->qid = qid;
    rctx->replier_id = replier_id;
    rctx->replier_kind = replier_kind;

    rctx->header = _Z_MID_REPLY_CONTEXT;
    if (is_final)
        _Z_SET_FLAG(rctx->header, _Z_FLAG_Z_F);

    return rctx;
}

void _z_msg_clear_reply_context(_z_reply_context_t *rc)
{
    if (!_Z_HAS_FLAG(rc->header, _Z_FLAG_Z_F))
        _z_bytes_clear(&rc->replier_id);
}

/*=============================*/
/*       Zenoh Messages        */
/*=============================*/
/*------------------ Resource Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_resource(_z_zint_t id, _z_reskey_t key)
{
    _z_declaration_t decl;

    decl.body.res.id = id;
    decl.body.res.key = key;

    decl.header = _Z_DECL_RESOURCE;
    if (decl.body.res.key.rname != NULL)
        _Z_SET_FLAG(decl.header, _Z_FLAG_Z_K);

    return decl;
}

void _z_msg_clear_declaration_resource(_z_res_decl_t *dcl)
{
    _z_reskey_clear(&dcl->key);
}

/*------------------ Forget Resource Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_forget_resource(_z_zint_t rid)
{
    _z_declaration_t decl;

    decl.body.forget_res.rid = rid;

    decl.header = _Z_DECL_FORGET_RESOURCE;

    return decl;
}

void _z_msg_clear_declaration_forget_resource(_z_forget_res_decl_t *dcl)
{
    (void)(dcl);
}

/*------------------ Publisher Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_publisher(_z_reskey_t key)
{
    _z_declaration_t decl;

    decl.body.pub.key = key;

    decl.header = _Z_DECL_PUBLISHER;
    if (key.rname != NULL)
        _Z_SET_FLAG(decl.header, _Z_FLAG_Z_K);

    return decl;
}

void _z_msg_clear_declaration_publisher(_z_pub_decl_t *dcl)
{
    _z_reskey_clear(&dcl->key);
}

/*------------------ Forget Publisher Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_forget_publisher(_z_reskey_t key)
{
    _z_declaration_t decl;

    decl.body.forget_pub.key = key;

    decl.header = _Z_DECL_FORGET_PUBLISHER;
    if (key.rname != NULL)
        _Z_SET_FLAG(decl.header, _Z_FLAG_Z_K);

    return decl;
}

void _z_msg_clear_declaration_forget_publisher(_z_forget_pub_decl_t *dcl)
{
    _z_reskey_clear(&dcl->key);
}

/*------------------ Subscriber Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_subscriber(_z_reskey_t key, _z_subinfo_t subinfo)
{
    _z_declaration_t decl;

    decl.body.sub.key = key;
    decl.body.sub.subinfo = subinfo;

    decl.header = _Z_DECL_SUBSCRIBER;
    if (key.rname)
        _Z_SET_FLAG(decl.header, _Z_FLAG_Z_K);
    if (subinfo.mode != Z_SUBMODE_PUSH)
        _Z_SET_FLAG(decl.header, _Z_FLAG_Z_S);
    if (subinfo.reliability == Z_RELIABILITY_RELIABLE)
        _Z_SET_FLAG(decl.header, _Z_FLAG_Z_R);

    return decl;
}

void _z_subinfo_clear(_z_subinfo_t *si)
{
    (void) (si);
    // Nothing to clear
}

void _z_msg_clear_declaration_subscriber(_z_sub_decl_t *dcl)
{
    _z_reskey_clear(&dcl->key);
    _z_subinfo_clear(&dcl->subinfo);
}

/*------------------ Forget Subscriber Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_forget_subscriber(_z_reskey_t key)
{
    _z_declaration_t decl;

    decl.body.forget_sub.key = key;

    decl.header = _Z_DECL_FORGET_SUBSCRIBER;
    if (key.rname != NULL)
        _Z_SET_FLAG(decl.header, _Z_FLAG_Z_K);

    return decl;
}

void _z_msg_clear_declaration_forget_subscriber(_z_forget_sub_decl_t *dcl)
{
    _z_reskey_clear(&dcl->key);
}

/*------------------ Queryable Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_queryable(_z_reskey_t key, _z_zint_t kind, _z_zint_t complete, _z_zint_t distance)
{
    _z_declaration_t decl;

    decl.body.qle.key = key;
    decl.body.qle.kind = kind;

    decl.header = _Z_DECL_QUERYABLE;
    if (key.rname != NULL)
        _Z_SET_FLAG(decl.header, _Z_FLAG_Z_K);

    decl.body.qle.complete = complete;
    decl.body.qle.distance = distance;
    if (decl.body.qle.complete != _Z_QUERYABLE_COMPLETE_DEFAULT || decl.body.qle.distance != _Z_QUERYABLE_DISTANCE_DEFAULT)
        _Z_SET_FLAG(decl.header, _Z_FLAG_Z_Q);

    return decl;
}

void _z_msg_clear_declaration_queryable(_z_qle_decl_t *dcl)
{
    _z_reskey_clear(&dcl->key);
}

/*------------------ Forget Queryable Declaration ------------------*/
_z_declaration_t _z_msg_make_declaration_forget_queryable(_z_reskey_t key, _z_zint_t kind)
{
    _z_declaration_t decl;

    decl.body.forget_qle.key = key;
    decl.body.forget_qle.kind = kind;

    decl.header = _Z_DECL_FORGET_QUERYABLE;
    if (key.rname != NULL)
        _Z_SET_FLAG(decl.header, _Z_FLAG_Z_K);

    return decl;
}

void _z_msg_clear_declaration_forget_queryable(_z_forget_qle_decl_t *dcl)
{
    _z_reskey_clear(&dcl->key);
}

/*------------------ Declare ------------------*/
_z_zenoh_message_t _z_msg_make_declare(_z_declaration_array_t declarations)
{
    _z_zenoh_message_t msg;

    msg.body.declare.declarations = declarations;

    msg.header = _Z_MID_DECLARE;

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

void _z_msg_clear_declaration(_z_declaration_t *dcl)
{
    uint8_t did = _Z_MID(dcl->header);
    switch (did)
    {
    case _Z_DECL_RESOURCE:
        _z_msg_clear_declaration_resource(&dcl->body.res);
        break;
    case _Z_DECL_PUBLISHER:
        _z_msg_clear_declaration_publisher(&dcl->body.pub);
        break;
    case _Z_DECL_SUBSCRIBER:
        _z_msg_clear_declaration_subscriber(&dcl->body.sub);
        break;
    case _Z_DECL_QUERYABLE:
        _z_msg_clear_declaration_queryable(&dcl->body.qle);
        break;
    case _Z_DECL_FORGET_RESOURCE:
        _z_msg_clear_declaration_forget_resource(&dcl->body.forget_res);
        break;
    case _Z_DECL_FORGET_PUBLISHER:
        _z_msg_clear_declaration_forget_publisher(&dcl->body.forget_pub);
        break;
    case _Z_DECL_FORGET_SUBSCRIBER:
        _z_msg_clear_declaration_forget_subscriber(&dcl->body.forget_sub);
        break;
    case _Z_DECL_FORGET_QUERYABLE:
        _z_msg_clear_declaration_forget_queryable(&dcl->body.forget_qle);
        break;
    default:
        _Z_DEBUG("WARNING: Trying to free declaration with unknown ID(%d)\n", did);
        break;
    }
}

void _z_msg_clear_declare(_z_msg_declare_t *dcl)
{
    _z_declaration_array_clear(&dcl->declarations);
}

/*------------------ Data Info Field ------------------*/
// @TODO: implement builder for _z_data_info_t

void _z_data_info_clear(_z_data_info_t *di)
{
    // NOTE: the following fiels do not involve any heap allocation:
    //   - source_sn
    //   - first_router_sn
    //   - kind

    if (_Z_HAS_FLAG(di->flags, _Z_DATA_INFO_ENC))
        _z_str_clear(di->encoding.suffix);

    if (_Z_HAS_FLAG(di->flags, _Z_DATA_INFO_SRC_ID))
        _z_bytes_clear(&di->source_id);

    if (_Z_HAS_FLAG(di->flags, _Z_DATA_INFO_RTR_ID))
        _z_bytes_clear(&di->first_router_id);

    if (_Z_HAS_FLAG(di->flags, _Z_DATA_INFO_TSTAMP))
        _z_timestamp_clear(&di->tstamp);
}

/*------------------ Data Message ------------------*/
_z_zenoh_message_t _z_msg_make_data(_z_reskey_t key, _z_data_info_t info, _z_payload_t payload, int can_be_dropped)
{
    _z_zenoh_message_t msg;

    msg.body.data.key = key;
    msg.body.data.info = info;
    msg.body.data.payload = payload;

    msg.header = _Z_MID_DATA;
    if (msg.body.data.info.flags != 0)
        _Z_SET_FLAG(msg.header, _Z_FLAG_Z_I);
    if (msg.body.data.key.rname != NULL)
        _Z_SET_FLAG(msg.header, _Z_FLAG_Z_K);
    if (can_be_dropped)
        _Z_SET_FLAG(msg.header, _Z_FLAG_Z_D);

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

void _z_msg_clear_data(_z_msg_data_t *msg)
{
    _z_reskey_clear(&msg->key);
    _z_data_info_clear(&msg->info);
    _z_payload_clear(&msg->payload);
}

/*------------------ Unit Message ------------------*/
_z_zenoh_message_t _z_msg_make_unit(int can_be_dropped)
{
    _z_zenoh_message_t msg;

    msg.header = _Z_MID_UNIT;
    if (can_be_dropped)
        _Z_SET_FLAG(msg.header, _Z_FLAG_Z_D);

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

void _z_msg_clear_unit(_z_msg_unit_t *unt)
{
    (void)(unt);
}

/*------------------ Pull Message ------------------*/
_z_zenoh_message_t _z_msg_make_pull(_z_reskey_t key, _z_zint_t pull_id, _z_zint_t max_samples, int is_final)
{
    _z_zenoh_message_t msg;

    msg.body.pull.key = key;
    msg.body.pull.pull_id = pull_id;
    msg.body.pull.max_samples = max_samples;

    msg.header = _Z_MID_PULL;
    if (is_final)
        _Z_SET_FLAG(msg.header, _Z_FLAG_Z_F);
    if (max_samples != 0)
        _Z_SET_FLAG(msg.header, _Z_FLAG_Z_N);
    if (msg.body.pull.key.rname != NULL)
        _Z_SET_FLAG(msg.header, _Z_FLAG_Z_K);

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

void _z_msg_clear_pull(_z_msg_pull_t *msg)
{
    _z_reskey_clear(&msg->key);
}

/*------------------ Query Message ------------------*/
_z_zenoh_message_t _z_msg_make_query(_z_reskey_t key, _z_str_t predicate, _z_zint_t qid, _z_query_target_t target, _z_consolidation_strategy_t consolidation)
{
    _z_zenoh_message_t msg;

    msg.body.query.key = key;
    msg.body.query.predicate = predicate;
    msg.body.query.qid = qid;
    msg.body.query.target = target;
    msg.body.query.consolidation = consolidation;

    msg.header = _Z_MID_QUERY;
    if (msg.body.query.target.kind != Z_QUERYABLE_ALL_KINDS)
        _Z_SET_FLAG(msg.header, _Z_FLAG_Z_T);
    if (msg.body.query.key.rname != NULL)
        _Z_SET_FLAG(msg.header, _Z_FLAG_Z_K);

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

void _z_msg_clear_query(_z_msg_query_t *msg)
{
    _z_reskey_clear(&msg->key);
    _z_str_clear(msg->predicate);
}

/*------------------ Reply Message ------------------*/
_z_zenoh_message_t _z_msg_make_reply(_z_reskey_t key, _z_data_info_t info, _z_payload_t payload, int can_be_dropped, _z_reply_context_t *rctx)
{
    _z_zenoh_message_t msg = _z_msg_make_data(key, info, payload, can_be_dropped);
    msg.reply_context = rctx;

    return msg;
}

/*------------------ Zenoh Message ------------------*/
void _z_msg_clear(_z_zenoh_message_t *msg)
{
    if (msg->attachment != NULL)
    {
        _z_t_msg_clear_attachment(msg->attachment);
        free(msg->attachment);
    }
    if (msg->reply_context != NULL)
    {
        _z_msg_clear_reply_context(msg->reply_context);
        free(msg->reply_context);
    }

    uint8_t mid = _Z_MID(msg->header);
    switch (mid)
    {
    case _Z_MID_DECLARE:
        _z_msg_clear_declare(&msg->body.declare);
        break;
    case _Z_MID_DATA:
        _z_msg_clear_data(&msg->body.data);
        break;
    case _Z_MID_PULL:
        _z_msg_clear_pull(&msg->body.pull);
        break;
    case _Z_MID_QUERY:
        _z_msg_clear_query(&msg->body.query);
        break;
    case _Z_MID_UNIT:
        _z_msg_clear_unit(&msg->body.unit);
        break;
    default:
        _Z_DEBUG("WARNING: Trying to encode message with unknown ID(%d)\n", mid);
        break;
    }
}

/*=============================*/
/*     Transport Messages      */
/*=============================*/
/*------------------ Scout Message ------------------*/
_z_transport_message_t _z_t_msg_make_scout(_z_zint_t what, int request_pid)
{
    _z_transport_message_t msg;

    msg.body.scout.what = what;

    msg.header = _Z_MID_SCOUT;
    if (request_pid)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_I);
    if (what != Z_ROUTER)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_W);

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_clear_scout(_z_t_msg_scout_t *msg, uint8_t header)
{
    // NOTE: scout does not involve any heap allocation
    (void)(msg);
    (void)(header);
}

/*------------------ Hello Message ------------------*/
_z_transport_message_t _z_t_msg_make_hello(_z_zint_t whatami, _z_bytes_t pid, _z_locator_array_t locators)
{
    _z_transport_message_t msg;

    msg.body.hello.whatami = whatami;
    msg.body.hello.pid = pid;
    msg.body.hello.locators = locators;

    msg.header = _Z_MID_HELLO;
    if (whatami != Z_ROUTER)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_W);
    if (!_z_bytes_is_empty(&pid))
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_I);
    if (!_z_locator_array_is_empty(&locators))
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_L);

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_clear_hello(_z_t_msg_hello_t *msg, uint8_t header)
{
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_I))
        _z_bytes_clear(&msg->pid);

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_L))
        _z_locators_clear(&msg->locators);
}

/*------------------ Join Message ------------------*/
_z_transport_message_t _z_t_msg_make_join(uint8_t version, _z_zint_t whatami, _z_zint_t lease, _z_zint_t sn_resolution, _z_bytes_t pid, _z_conduit_sn_list_t next_sns)
{
    _z_transport_message_t msg;

    msg.body.join.options = 0;
    if (next_sns.is_qos)
        _Z_SET_FLAG(msg.body.join.options, _Z_OPT_JOIN_QOS);
    msg.body.join.version = version;
    msg.body.join.whatami = whatami;
    msg.body.join.lease = lease;
    msg.body.join.sn_resolution = sn_resolution;
    msg.body.join.next_sns = next_sns;
    msg.body.join.pid = pid;

    msg.header = _Z_MID_JOIN;
    if (lease % 1000 == 0)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_T1);
    if (sn_resolution != Z_SN_RESOLUTION_DEFAULT)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_S);
    if (msg.body.join.options != 0)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_O);

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_copy_join(_z_t_msg_join_t *clone, _z_t_msg_join_t *msg)
{
    clone->options = msg->options;
    clone->version = msg->version;
    clone->whatami = msg->whatami;
    clone->lease = msg->lease;
    clone->sn_resolution = msg->sn_resolution;
    clone->next_sns = msg->next_sns;
    _z_bytes_copy(&clone->pid, &msg->pid);
}

void _z_t_msg_clear_join(_z_t_msg_join_t *msg, uint8_t header)
{
    (void) (header);
    _z_bytes_clear(&msg->pid);
}

/*------------------ Init Message ------------------*/
_z_transport_message_t _z_t_msg_make_init_syn(uint8_t version, _z_zint_t whatami, _z_zint_t sn_resolution, _z_bytes_t pid, int is_qos)
{
    _z_transport_message_t msg;

    msg.body.init.options = 0;
    if (is_qos)
        _Z_SET_FLAG(msg.body.init.options, _Z_OPT_INIT_QOS);
    msg.body.init.version = version;
    msg.body.init.whatami = whatami;
    msg.body.init.sn_resolution = sn_resolution;
    msg.body.init.pid = pid;
    _z_bytes_reset(&msg.body.init.cookie);

    msg.header = _Z_MID_INIT;
    if (sn_resolution != Z_SN_RESOLUTION_DEFAULT)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_S);
    if (msg.body.init.options != 0)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_O);

    msg.attachment = NULL;

    return msg;
}

_z_transport_message_t _z_t_msg_make_init_ack(uint8_t version, _z_zint_t whatami, _z_zint_t sn_resolution, _z_bytes_t pid, _z_bytes_t cookie, int is_qos)
{
    _z_transport_message_t msg;

    msg.body.init.options = 0;
    if (is_qos)
        _Z_SET_FLAG(msg.body.init.options, _Z_OPT_INIT_QOS);
    msg.body.init.version = version;
    msg.body.init.whatami = whatami;
    msg.body.init.sn_resolution = sn_resolution;
    msg.body.init.pid = pid;
    msg.body.init.cookie = cookie;

    msg.header = _Z_MID_INIT;
    _Z_SET_FLAG(msg.header, _Z_FLAG_T_A);
    if (sn_resolution != Z_SN_RESOLUTION_DEFAULT)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_S);
    if (msg.body.init.options != 0)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_O);

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_copy_init(_z_t_msg_init_t *clone, _z_t_msg_init_t *msg)
{
    clone->options = msg->options;
    clone->version = msg->version;
    clone->whatami = msg->whatami;
    clone->sn_resolution = msg->sn_resolution;
    _z_bytes_copy(&clone->pid, &msg->pid);
    _z_bytes_copy(&clone->cookie, &msg->cookie);
}

void _z_t_msg_clear_init(_z_t_msg_init_t *msg, uint8_t header)
{
    _z_bytes_clear(&msg->pid);
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_A))
        _z_bytes_clear(&msg->cookie);
}

/*------------------ Open Message ------------------*/
_z_transport_message_t _z_t_msg_make_open_syn(_z_zint_t lease, _z_zint_t initial_sn, _z_bytes_t cookie)
{
    _z_transport_message_t msg;

    msg.body.open.lease = lease;
    msg.body.open.initial_sn = initial_sn;
    msg.body.open.cookie = cookie;

    msg.header = _Z_MID_OPEN;
    if (lease % 1000 == 0)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_T2);

    msg.attachment = NULL;

    return msg;
}

_z_transport_message_t _z_t_msg_make_open_ack(_z_zint_t lease, _z_zint_t initial_sn)
{
    _z_transport_message_t msg;

    msg.body.open.lease = lease;
    msg.body.open.initial_sn = initial_sn;
    _z_bytes_reset(&msg.body.open.cookie);

    msg.header = _Z_MID_OPEN;
    _Z_SET_FLAG(msg.header, _Z_FLAG_T_A);
    if (lease % 1000 == 0)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_T2);

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_copy_open(_z_t_msg_open_t *clone, _z_t_msg_open_t *msg)
{
    clone->lease = msg->lease;
    clone->initial_sn = msg->initial_sn;
    _z_bytes_reset(&clone->cookie);
}

void _z_t_msg_clear_open(_z_t_msg_open_t *msg, uint8_t header)
{
    if (!_Z_HAS_FLAG(header, _Z_FLAG_T_A))
        _z_bytes_clear(&msg->cookie);
}

/*------------------ Close Message ------------------*/
_z_transport_message_t _z_t_msg_make_close(uint8_t reason, _z_bytes_t pid, int link_only)
{
    _z_transport_message_t msg;

    msg.body.close.reason = reason;
    msg.body.close.pid = pid;

    msg.header = _Z_MID_CLOSE;
    if (!_z_bytes_is_empty(&pid))
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_I);
    if (link_only)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_K);

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_clear_close(_z_t_msg_close_t *msg, uint8_t header)
{
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_I))
        _z_bytes_clear(&msg->pid);
}

/*------------------ Sync Message ------------------*/
_z_transport_message_t _z_t_msg_make_sync(_z_zint_t sn, int is_reliable, _z_zint_t count)
{
    _z_transport_message_t msg;

    msg.body.sync.sn = sn;
    msg.body.sync.count = count;

    msg.header = _Z_MID_SYNC;
    if (is_reliable)
    {
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_R);
        if (count != 0)
            _Z_SET_FLAG(msg.header, _Z_FLAG_T_C);
    }

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_clear_sync(_z_t_msg_sync_t *msg, uint8_t header)
{
    // NOTE: sync does not involve any heap allocation
    (void)(msg);
    (void)(header);
}

/*------------------ AckNack Message ------------------*/
_z_transport_message_t _z_t_msg_make_ack_nack(_z_zint_t sn, _z_zint_t mask)
{
    _z_transport_message_t msg;

    msg.body.ack_nack.sn = sn;
    msg.body.ack_nack.mask = mask;

    msg.header = _Z_MID_ACK_NACK;
    if (mask != 0)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_M);

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_clear_ack_nack(_z_t_msg_ack_nack_t *msg, uint8_t header)
{
    // NOTE: ack_nack does not involve any heap allocation
    (void)(msg);
    (void)(header);
}

/*------------------ Keep Alive Message ------------------*/
_z_transport_message_t _z_t_msg_make_keep_alive(_z_bytes_t pid)
{
    _z_transport_message_t msg;

    msg.body.keep_alive.pid = pid;

    msg.header = _Z_MID_KEEP_ALIVE;
    if (!_z_bytes_is_empty(&pid))
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_I);

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_clear_keep_alive(_z_t_msg_keep_alive_t *msg, uint8_t header)
{
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_I))
        _z_bytes_clear(&msg->pid);
}

/*------------------ PingPong Messages ------------------*/
_z_transport_message_t _z_t_msg_make_ping(_z_zint_t hash)
{
    _z_transport_message_t msg;

    msg.body.ping_pong.hash = hash;

    msg.header = _Z_MID_PING_PONG;

    msg.attachment = NULL;

    return msg;
}

_z_transport_message_t _z_t_msg_make_pong(_z_zint_t hash)
{
    _z_transport_message_t msg;

    msg.body.ping_pong.hash = hash;

    msg.header = _Z_MID_PING_PONG;
    _Z_SET_FLAG(msg.header, _Z_FLAG_T_P);

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_clear_ping_pong(_z_t_msg_ping_pong_t *msg, uint8_t header)
{
    // NOTE: ping_pong does not involve any heap allocation
    (void)(header);
    (void)(msg);
}

/*------------------ Frame Message ------------------*/
_z_transport_message_t _z_t_msg_make_frame_header(_z_zint_t sn, int is_reliable, int is_fragment, int is_final)
{
    _z_transport_message_t msg;

    msg.body.frame.sn = sn;

    // Reset payload content
    memset(&msg.body.frame.payload, 0, sizeof(_z_frame_payload_t));

    msg.header = _Z_MID_FRAME;
    if (is_reliable)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_R);
    if (is_fragment)
    {
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_F);
        if (is_final)
            _Z_SET_FLAG(msg.header, _Z_FLAG_T_E);
    }

    msg.attachment = NULL;

    return msg;
}

_z_transport_message_t _z_t_msg_make_frame(_z_zint_t sn, _z_frame_payload_t payload, int is_reliable, int is_fragment, int is_final)
{
    _z_transport_message_t msg;

    msg.body.frame.sn = sn;
    msg.body.frame.payload = payload;

    msg.header = _Z_MID_FRAME;
    if (is_reliable)
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_R);
    if (is_fragment)
    {
        _Z_SET_FLAG(msg.header, _Z_FLAG_T_F);
        if (is_final)
            _Z_SET_FLAG(msg.header, _Z_FLAG_T_E);
    }

    msg.attachment = NULL;

    return msg;
}

void _z_t_msg_clear_frame(_z_t_msg_frame_t *msg, uint8_t header)
{
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_F))
        _z_payload_clear(&msg->payload.fragment);
    else
        _z_zenoh_message_vec_clear(&msg->payload.messages);
}

/*------------------ Transport Message ------------------*/
void _z_t_msg_copy(_z_transport_message_t *clone, _z_transport_message_t *msg)
{
    clone->header = msg->header;
    clone->attachment = msg->attachment;

    uint8_t mid = _Z_MID(msg->header);
    switch (mid)
    {
    case _Z_MID_SCOUT:
        // _z_t_msg_copy_scout(&clone->body.scout, &msg->body.scout);
        break;
    case _Z_MID_HELLO:
        // _z_t_msg_copy_hello(&clone->body.hello, &msg->body.hello);
        break;
    case _Z_MID_JOIN:
        _z_t_msg_copy_join(&clone->body.join, &msg->body.join);
        break;
    case _Z_MID_INIT:
        _z_t_msg_copy_init(&clone->body.init, &msg->body.init);
        break;
    case _Z_MID_OPEN:
        _z_t_msg_copy_open(&clone->body.open, &msg->body.open);
        break;
    case _Z_MID_CLOSE:
        // _z_t_msg_copy_close(&clone->body.close, &msg->body.close);
        break;
    case _Z_MID_SYNC:
        // _z_t_msg_copy_sync(&clone->body.sync, (&msg->body.sync);
        break;
    case _Z_MID_ACK_NACK:
        // _z_t_msg_copy_ack_nack(&clone->body.ack_nack, g->body.ack_nack);
        break;
    case _Z_MID_KEEP_ALIVE:
        // _z_t_msg_copy_keep_alive(&clone->body.keep_alive, >body.keep_alive);
        break;
    case _Z_MID_PING_PONG:
        // _z_t_msg_copy_ping_pong(&clone->body.ping_pong, ->body.ping_pong);
        break;
    case _Z_MID_FRAME:
        // _z_t_msg_copy_frame(&clone->body.frame, &msg->body.frame);
        break;
    default:
        _Z_DEBUG("WARNING: Trying to free session message with unknown ID(%d)\n", mid);
        break;
    }
}

void _z_t_msg_clear(_z_transport_message_t *msg)
{
    if (msg->attachment)
    {
        _z_t_msg_clear_attachment(msg->attachment);
        free(msg->attachment);
    }

    uint8_t mid = _Z_MID(msg->header);
    switch (mid)
    {
    case _Z_MID_SCOUT:
        _z_t_msg_clear_scout(&msg->body.scout, msg->header);
        break;
    case _Z_MID_HELLO:
        _z_t_msg_clear_hello(&msg->body.hello, msg->header);
        break;
    case _Z_MID_JOIN:
        _z_t_msg_clear_join(&msg->body.join, msg->header);
        break;
    case _Z_MID_INIT:
        _z_t_msg_clear_init(&msg->body.init, msg->header);
        break;
    case _Z_MID_OPEN:
        _z_t_msg_clear_open(&msg->body.open, msg->header);
        break;
    case _Z_MID_CLOSE:
        _z_t_msg_clear_close(&msg->body.close, msg->header);
        break;
    case _Z_MID_SYNC:
        _z_t_msg_clear_sync(&msg->body.sync, msg->header);
        break;
    case _Z_MID_ACK_NACK:
        _z_t_msg_clear_ack_nack(&msg->body.ack_nack, msg->header);
        break;
    case _Z_MID_KEEP_ALIVE:
        _z_t_msg_clear_keep_alive(&msg->body.keep_alive, msg->header);
        break;
    case _Z_MID_PING_PONG:
        _z_t_msg_clear_ping_pong(&msg->body.ping_pong, msg->header);
        break;
    case _Z_MID_FRAME:
        _z_t_msg_clear_frame(&msg->body.frame, msg->header);
        return;
    default:
        _Z_DEBUG("WARNING: Trying to free session message with unknown ID(%d)\n", mid);
        return;
    }
}
