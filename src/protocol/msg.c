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

#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/utils/logging.h"

/*=============================*/
/*        Message fields       */
/*=============================*/
/*------------------ Payload field ------------------*/
void _zn_payload_clear(_zn_payload_t *p)
{
    _z_bytes_clear(p);
}

/*------------------ Timestamp Field ------------------*/
void z_timestamp_clear(z_timestamp_t *ts)
{
    _z_bytes_clear(&ts->id);
}

/*------------------ ResKey Field ------------------*/
void _zn_reskey_clear(zn_reskey_t *rk)
{
    rk->rid = 0;
    if (rk->rname != NULL)
    {
        free(rk->rname);
        rk->rname = NULL;
    }
}

/*------------------ Locators Field ------------------*/
void _zn_locators_clear(_zn_locator_array_t *ls)
{
    _zn_locator_array_clear(ls);
}

/*=============================*/
/*      Message decorators     */
/*=============================*/
/*------------------ Attachment Decorator ------------------*/
void _zn_t_msg_clear_attachment(_zn_attachment_t *a)
{
    _zn_payload_clear(&a->payload);
}

/*------------------ ReplyContext Decorator ------------------*/
_zn_reply_context_t *_zn_z_msg_make_reply_context(z_zint_t qid, z_bytes_t replier_id, z_zint_t replier_kind, int is_final)
{
    _zn_reply_context_t *rctx = (_zn_reply_context_t *)malloc(sizeof(_zn_reply_context_t));

    rctx->qid = qid;
    rctx->replier_id = replier_id;
    rctx->replier_kind = replier_kind;

    rctx->header = _ZN_MID_REPLY_CONTEXT;
    if (is_final)
        _ZN_SET_FLAG(rctx->header, _ZN_FLAG_Z_F);

    return rctx;
}

void _zn_z_msg_clear_reply_context(_zn_reply_context_t *rc)
{
    if (!_ZN_HAS_FLAG(rc->header, _ZN_FLAG_Z_F))
        _z_bytes_clear(&rc->replier_id);
}

/*=============================*/
/*       Zenoh Messages        */
/*=============================*/
/*------------------ Resource Declaration ------------------*/
_zn_declaration_t _zn_z_msg_make_declaration_resource(z_zint_t id, zn_reskey_t key)
{
    _zn_declaration_t decl;

    decl.body.res.id = id;
    decl.body.res.key = key;

    decl.header = _ZN_DECL_RESOURCE;
    if (decl.body.res.key.rname != NULL)
        _ZN_SET_FLAG(decl.header, _ZN_FLAG_Z_K);

    return decl;
}

void _zn_z_msg_clear_declaration_resource(_zn_res_decl_t *dcl)
{
    _zn_reskey_clear(&dcl->key);
}

/*------------------ Forget Resource Declaration ------------------*/
_zn_declaration_t _zn_z_msg_make_declaration_forget_resource(z_zint_t rid)
{
    _zn_declaration_t decl;

    decl.body.forget_res.rid = rid;

    decl.header = _ZN_DECL_FORGET_RESOURCE;

    return decl;
}

void _zn_z_msg_clear_declaration_forget_resource(_zn_forget_res_decl_t *dcl)
{
    (void)(dcl);
}

/*------------------ Publisher Declaration ------------------*/
_zn_declaration_t _zn_z_msg_make_declaration_publisher(zn_reskey_t key)
{
    _zn_declaration_t decl;

    decl.body.pub.key = key;

    decl.header = _ZN_DECL_PUBLISHER;
    if (key.rname != NULL)
        _ZN_SET_FLAG(decl.header, _ZN_FLAG_Z_K);

    return decl;
}

void _zn_z_msg_clear_declaration_publisher(_zn_pub_decl_t *dcl)
{
    _zn_reskey_clear(&dcl->key);
}

/*------------------ Forget Publisher Declaration ------------------*/
_zn_declaration_t _zn_z_msg_make_declaration_forget_publisher(zn_reskey_t key)
{
    _zn_declaration_t decl;

    decl.body.forget_pub.key = key;

    decl.header = _ZN_DECL_FORGET_PUBLISHER;
    if (key.rname != NULL)
        _ZN_SET_FLAG(decl.header, _ZN_FLAG_Z_K);

    return decl;
}

void _zn_z_msg_clear_declaration_forget_publisher(_zn_forget_pub_decl_t *dcl)
{
    _zn_reskey_clear(&dcl->key);
}

/*------------------ Subscriber Declaration ------------------*/
_zn_declaration_t _zn_z_msg_make_declaration_subscriber(zn_reskey_t key, zn_subinfo_t subinfo)
{
    _zn_declaration_t decl;

    decl.body.sub.key = key;
    decl.body.sub.subinfo = subinfo;

    decl.header = _ZN_DECL_SUBSCRIBER;
    if (key.rname)
        _ZN_SET_FLAG(decl.header, _ZN_FLAG_Z_K);
    if (subinfo.mode != zn_submode_t_PUSH || subinfo.period)
        _ZN_SET_FLAG(decl.header, _ZN_FLAG_Z_S);
    if (subinfo.reliability == zn_reliability_t_RELIABLE)
        _ZN_SET_FLAG(decl.header, _ZN_FLAG_Z_R);

    return decl;
}

void _zn_subinfo_clear(zn_subinfo_t *si)
{
    if (si->period)
        free(si->period);
}

void _zn_z_msg_clear_declaration_subscriber(_zn_sub_decl_t *dcl)
{
    _zn_reskey_clear(&dcl->key);
    _zn_subinfo_clear(&dcl->subinfo);
}

/*------------------ Forget Subscriber Declaration ------------------*/
_zn_declaration_t _zn_z_msg_make_declaration_forget_subscriber(zn_reskey_t key)
{
    _zn_declaration_t decl;

    decl.body.forget_sub.key = key;

    decl.header = _ZN_DECL_FORGET_SUBSCRIBER;
    if (key.rname != NULL)
        _ZN_SET_FLAG(decl.header, _ZN_FLAG_Z_K);

    return decl;
}

void _zn_z_msg_clear_declaration_forget_subscriber(_zn_forget_sub_decl_t *dcl)
{
    _zn_reskey_clear(&dcl->key);
}

/*------------------ Queryable Declaration ------------------*/
_zn_declaration_t _zn_z_msg_make_declaration_queryable(zn_reskey_t key, z_zint_t kind, z_zint_t complete, z_zint_t distance)
{
    _zn_declaration_t decl;

    decl.body.qle.key = key;
    decl.body.qle.kind = kind;

    decl.header = _ZN_DECL_QUERYABLE;
    if (key.rname != NULL)
        _ZN_SET_FLAG(decl.header, _ZN_FLAG_Z_K);

    decl.body.qle.complete = complete;
    decl.body.qle.distance = distance;
    if (decl.body.qle.complete != _ZN_QUERYABLE_COMPLETE_DEFAULT || decl.body.qle.distance != _ZN_QUERYABLE_DISTANCE_DEFAULT)
        _ZN_SET_FLAG(decl.header, _ZN_FLAG_Z_Q);

    return decl;
}

void _zn_z_msg_clear_declaration_queryable(_zn_qle_decl_t *dcl)
{
    _zn_reskey_clear(&dcl->key);
}

/*------------------ Forget Queryable Declaration ------------------*/
_zn_declaration_t _zn_z_msg_make_declaration_forget_queryable(zn_reskey_t key, z_zint_t kind)
{
    _zn_declaration_t decl;

    decl.body.forget_qle.key = key;
    decl.body.forget_qle.kind = kind;

    decl.header = _ZN_DECL_FORGET_QUERYABLE;
    if (key.rname != NULL)
        _ZN_SET_FLAG(decl.header, _ZN_FLAG_Z_K);

    return decl;
}

void _zn_z_msg_clear_declaration_forget_queryable(_zn_forget_qle_decl_t *dcl)
{
    _zn_reskey_clear(&dcl->key);
}

/*------------------ Declare ------------------*/
_zn_zenoh_message_t _zn_z_msg_make_declare(_zn_declaration_array_t declarations)
{
    _zn_zenoh_message_t msg;

    msg.body.declare.declarations = declarations;

    msg.header = _ZN_MID_DECLARE;

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

void _zn_z_msg_clear_declaration(_zn_declaration_t *dcl)
{
    uint8_t did = _ZN_MID(dcl->header);
    switch (did)
    {
    case _ZN_DECL_RESOURCE:
        _zn_z_msg_clear_declaration_resource(&dcl->body.res);
        break;
    case _ZN_DECL_PUBLISHER:
        _zn_z_msg_clear_declaration_publisher(&dcl->body.pub);
        break;
    case _ZN_DECL_SUBSCRIBER:
        _zn_z_msg_clear_declaration_subscriber(&dcl->body.sub);
        break;
    case _ZN_DECL_QUERYABLE:
        _zn_z_msg_clear_declaration_queryable(&dcl->body.qle);
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        _zn_z_msg_clear_declaration_forget_resource(&dcl->body.forget_res);
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        _zn_z_msg_clear_declaration_forget_publisher(&dcl->body.forget_pub);
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        _zn_z_msg_clear_declaration_forget_subscriber(&dcl->body.forget_sub);
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        _zn_z_msg_clear_declaration_forget_queryable(&dcl->body.forget_qle);
        break;
    default:
        _Z_ERROR("WARNING: Trying to free declaration with unknown ID(%d)\n", did);
        break;
    }
}

void _zn_z_msg_clear_declare(_zn_declare_t *dcl)
{
    _zn_declaration_array_clear(&dcl->declarations);
}

/*------------------ Data Info Field ------------------*/
// @TODO: implement builder for _zn_data_info_t

void _zn_data_info_clear(_zn_data_info_t *di)
{
    // NOTE: the following fiels do not involve any heap allocation:
    //   - source_sn
    //   - first_router_sn
    //   - kind

    if (_ZN_HAS_FLAG(di->flags, _ZN_DATA_INFO_ENC))
        _z_str_clear(di->encoding.suffix);

    if (_ZN_HAS_FLAG(di->flags, _ZN_DATA_INFO_SRC_ID))
        _z_bytes_clear(&di->source_id);

    if (_ZN_HAS_FLAG(di->flags, _ZN_DATA_INFO_RTR_ID))
        _z_bytes_clear(&di->first_router_id);

    if (_ZN_HAS_FLAG(di->flags, _ZN_DATA_INFO_TSTAMP))
        z_timestamp_clear(&di->tstamp);
}

/*------------------ Data Message ------------------*/
_zn_zenoh_message_t _zn_z_msg_make_data(zn_reskey_t key, _zn_data_info_t info, _zn_payload_t payload, int can_be_dropped)
{
    _zn_zenoh_message_t msg;

    msg.body.data.key = key;
    msg.body.data.info = info;
    msg.body.data.payload = payload;

    msg.header = _ZN_MID_DATA;
    if (msg.body.data.info.flags != 0)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_Z_I);
    if (msg.body.data.key.rname != NULL)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_Z_K);
    if (can_be_dropped)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_Z_D);

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

void _zn_z_msg_clear_data(_zn_data_t *msg)
{
    _zn_reskey_clear(&msg->key);
    _zn_data_info_clear(&msg->info);
    _zn_payload_clear(&msg->payload);
}

/*------------------ Unit Message ------------------*/
_zn_zenoh_message_t _zn_z_msg_make_unit(int can_be_dropped)
{
    _zn_zenoh_message_t msg;

    msg.header = _ZN_MID_UNIT;
    if (can_be_dropped)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_Z_D);

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

void _zn_z_msg_clear_unit(_zn_unit_t *unt)
{
    (void)(unt);
}

/*------------------ Pull Message ------------------*/
_zn_zenoh_message_t _zn_z_msg_make_pull(zn_reskey_t key, z_zint_t pull_id, z_zint_t max_samples, int is_final)
{
    _zn_zenoh_message_t msg;

    msg.body.pull.key = key;
    msg.body.pull.pull_id = pull_id;
    msg.body.pull.max_samples = max_samples;

    msg.header = _ZN_MID_PULL;
    if (is_final)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_Z_F);
    if (max_samples != 0)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_Z_N);
    if (msg.body.pull.key.rname != NULL)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_Z_K);

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

void _zn_z_msg_clear_pull(_zn_pull_t *msg)
{
    _zn_reskey_clear(&msg->key);
}

/*------------------ Query Message ------------------*/
_zn_zenoh_message_t _zn_z_msg_make_query(zn_reskey_t key, z_str_t predicate, z_zint_t qid, zn_query_target_t target, zn_query_consolidation_t consolidation)
{
    _zn_zenoh_message_t msg;

    msg.body.query.key = key;
    msg.body.query.predicate = predicate;
    msg.body.query.qid = qid;
    msg.body.query.target = target;
    msg.body.query.consolidation = consolidation;

    msg.header = _ZN_MID_QUERY;
    if (msg.body.query.target.kind != ZN_QUERYABLE_ALL_KINDS)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_Z_T);
    if (msg.body.query.key.rname != NULL)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_Z_K);

    msg.attachment = NULL;
    msg.reply_context = NULL;

    return msg;
}

void _zn_z_msg_clear_query(_zn_query_t *msg)
{
    _zn_reskey_clear(&msg->key);
    _z_str_clear(msg->predicate);
}

/*------------------ Reply Message ------------------*/
_zn_zenoh_message_t _zn_z_msg_make_reply(zn_reskey_t key, _zn_data_info_t info, _zn_payload_t payload, int can_be_dropped, _zn_reply_context_t *rctx)
{
    _zn_zenoh_message_t msg = _zn_z_msg_make_data(key, info, payload, can_be_dropped);
    msg.reply_context = rctx;

    return msg;
}

/*------------------ Zenoh Message ------------------*/
void _zn_z_msg_clear(_zn_zenoh_message_t *msg)
{
    if (msg->attachment != NULL)
    {
        _zn_t_msg_clear_attachment(msg->attachment);
        free(msg->attachment);
    }
    if (msg->reply_context != NULL)
    {
        _zn_z_msg_clear_reply_context(msg->reply_context);
        free(msg->reply_context);
    }

    uint8_t mid = _ZN_MID(msg->header);
    switch (mid)
    {
    case _ZN_MID_DECLARE:
        _zn_z_msg_clear_declare(&msg->body.declare);
        break;
    case _ZN_MID_DATA:
        _zn_z_msg_clear_data(&msg->body.data);
        break;
    case _ZN_MID_PULL:
        _zn_z_msg_clear_pull(&msg->body.pull);
        break;
    case _ZN_MID_QUERY:
        _zn_z_msg_clear_query(&msg->body.query);
        break;
    case _ZN_MID_UNIT:
        _zn_z_msg_clear_unit(&msg->body.unit);
        break;
    default:
        _Z_ERROR("WARNING: Trying to encode message with unknown ID(%d)\n", mid);
        break;
    }
}

/*=============================*/
/*     Transport Messages      */
/*=============================*/
/*------------------ Scout Message ------------------*/
_zn_transport_message_t _zn_t_msg_make_scout(uint8_t what, z_bytes_t zid)
{
    _zn_transport_message_t msg;

    msg.header = _ZN_MID_SCOUT;
    msg.body.scout.version = ZN_PROTO_VERSION;
    msg.body.scout.what = what;
    msg.body.scout.zid = zid;

    if (!_z_bytes_is_empty(&zid))
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_HDR_SCOUT_I);

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_copy_scout(_zn_scout_t *clone, _zn_scout_t *scout)
{
    clone->version = scout->version;
    clone->what = scout->what;
    _z_bytes_copy(&clone->zid, &scout->zid);
}

void _zn_t_msg_clear_scout(_zn_scout_t *msg, uint8_t header)
{
    if (!_z_bytes_is_empty(&msg->zid))
        _z_bytes_clear(&msg->zid);
}

/*------------------ Hello Message ------------------*/
_zn_transport_message_t _zn_t_msg_make_hello(uint8_t whatami, z_bytes_t zid, _zn_locator_array_t locators)
{
    _zn_transport_message_t msg;

    msg.header = _ZN_MID_HELLO;
    msg.body.hello.version = ZN_PROTO_VERSION;
    msg.body.hello.whatami = whatami;
    msg.body.hello.zid = zid;
    msg.body.hello.locators = locators;

    if (!_zn_locator_array_is_empty(&locators))
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_HDR_HELLO_L);

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_copy_hello(_zn_hello_t *clone, _zn_hello_t *hello)
{
    clone->version = hello->version;
    clone->whatami = hello->whatami;
    _z_bytes_copy(&clone->zid, &hello->zid);
    // clone->locators = _zn_locator_array_clone(&hello->locators); // TODO: implement clone
    _z_bytes_copy(&clone->zid, &hello->zid);
}

void _zn_t_msg_clear_hello(_zn_hello_t *msg, uint8_t header)
{
    _z_bytes_clear(&msg->zid);

    if (_ZN_HAS_FLAG(header, _ZN_FLAG_HDR_HELLO_L))
        _zn_locators_clear(&msg->locators);
}

/*------------------ Join Message ------------------*/
_zn_transport_message_t _zn_t_msg_make_join(uint8_t version, uint8_t whatami, z_zint_t lease, uint8_t sn_bs, z_bytes_t zid, _zn_conduit_sn_t next_sn)
{
    _zn_transport_message_t msg;

    msg.header = _ZN_MID_JOIN;
    msg.body.join.version = version;
    msg.body.join.whatami = whatami & 0x03;
    msg.body.join.sn_bs = sn_bs & 0x07;
    msg.body.join.lease = lease;
    if (lease % 1000 == 0)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_HDR_JOIN_T);
    msg.body.join.next_sn = next_sn;
    msg.body.join.zid = zid;

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_copy_join(_zn_join_t *clone, _zn_join_t *msg)
{
    clone->version = msg->version;
    clone->whatami = msg->whatami;
    clone->sn_bs = msg->sn_bs;
    clone->lease = msg->lease;
    clone->next_sn = msg->next_sn;
    _z_bytes_copy(&clone->zid, &msg->zid);
}

void _zn_t_msg_clear_join(_zn_join_t *msg, uint8_t header)
{
    _z_bytes_clear(&msg->zid);
}

/*------------------ Init Message ------------------*/
_zn_transport_message_t _zn_t_msg_make_init_syn(uint8_t version, uint8_t whatami, uint8_t sn_bs, z_bytes_t zid)
{
    _zn_transport_message_t msg;

    msg.header = _ZN_MID_INIT;
    msg.body.init.version = version;
    msg.body.init.whatami = whatami & 0x03;
    msg.body.init.sn_bs = sn_bs & 0x07;
    msg.body.init.zid = zid;
    _z_bytes_reset(&msg.body.init.cookie);

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_init_ack(uint8_t version, uint8_t whatami, uint8_t sn_bs, z_bytes_t zid, z_bytes_t cookie)
{
    _zn_transport_message_t msg;

    msg.header = _ZN_MID_INIT;
    _ZN_SET_FLAG(msg.header, _ZN_FLAG_HDR_INIT_A);
    msg.body.init.version = version;
    msg.body.init.whatami = whatami & 0x03;
    msg.body.init.sn_bs = sn_bs & 0x07;
    msg.body.init.zid = zid;
    msg.body.init.cookie = cookie;

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_copy_init(_zn_init_t *clone, _zn_init_t *msg)
{
    clone->version = msg->version;
    clone->whatami = msg->whatami;
    clone->sn_bs = msg->sn_bs;
    _z_bytes_copy(&clone->zid, &msg->zid);
    _z_bytes_copy(&clone->cookie, &msg->cookie);
}

void _zn_t_msg_clear_init(_zn_init_t *msg, uint8_t header)
{
    _z_bytes_clear(&msg->zid);
    if (_ZN_HAS_FLAG(header, _ZN_FLAG_T_A))
        _z_bytes_clear(&msg->cookie);
}

/*------------------ Open Message ------------------*/
_zn_transport_message_t _zn_t_msg_make_open_syn(z_zint_t lease, z_zint_t initial_sn, z_bytes_t cookie)
{
    _zn_transport_message_t msg;

    msg.header = _ZN_MID_OPEN;
    msg.body.open.lease = lease;
    msg.body.open.initial_sn = initial_sn;
    msg.body.open.cookie = cookie;
    if (lease % 1000 == 0)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_HDR_OPEN_T);

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_open_ack(z_zint_t lease, z_zint_t initial_sn)
{
    _zn_transport_message_t msg;

    msg.header = _ZN_MID_OPEN;
    _ZN_SET_FLAG(msg.header, _ZN_FLAG_HDR_OPEN_A);
    msg.body.open.lease = lease;
    msg.body.open.initial_sn = initial_sn;
    _z_bytes_reset(&msg.body.open.cookie);

    if (lease % 1000 == 0)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_HDR_OPEN_T);

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_copy_open(_zn_open_t *clone, _zn_open_t *msg)
{
    clone->lease = msg->lease;
    clone->initial_sn = msg->initial_sn;
    _z_bytes_copy(&clone->cookie, &msg->cookie);
}

void _zn_t_msg_clear_open(_zn_open_t *msg, uint8_t header)
{
    if (!_ZN_HAS_FLAG(header, _ZN_FLAG_T_A))
        _z_bytes_clear(&msg->cookie);
}

/*------------------ Close Message ------------------*/
_zn_transport_message_t _zn_t_msg_make_close(uint8_t reason)
{
    _zn_transport_message_t msg;

    msg.header = _ZN_MID_CLOSE;
    msg.body.close.reason = reason;

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_copy_close(_zn_close_t *clone, _zn_close_t *close)
{
    clone->reason = close->reason;
}

void _zn_t_msg_clear_close(_zn_close_t *msg, uint8_t header)
{
}

/*------------------ Sync Message ------------------*/
_zn_transport_message_t _zn_t_msg_make_sync(z_zint_t sn, int is_reliable, z_zint_t count)
{
    _zn_transport_message_t msg;

    msg.body.sync.sn = sn;
    msg.body.sync.count = count;

    msg.header = _ZN_MID_SYNC;
    if (is_reliable)
    {
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_T_R);
        if (count != 0)
            _ZN_SET_FLAG(msg.header, _ZN_FLAG_T_C);
    }

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_clear_sync(_zn_sync_t *msg, uint8_t header)
{
    // NOTE: sync does not involve any heap allocation
    (void)(msg);
    (void)(header);
}

/*------------------ AckNack Message ------------------*/
_zn_transport_message_t _zn_t_msg_make_ack_nack(z_zint_t sn, z_zint_t mask)
{
    _zn_transport_message_t msg;

    msg.body.ack_nack.sn = sn;
    msg.body.ack_nack.mask = mask;

    msg.header = _ZN_MID_ACK_NACK;
    if (mask != 0)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_T_M);

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_clear_ack_nack(_zn_ack_nack_t *msg, uint8_t header)
{
    // NOTE: ack_nack does not involve any heap allocation
    (void)(msg);
    (void)(header);
}

/*------------------ Keep Alive Message ------------------*/
_zn_transport_message_t _zn_t_msg_make_keep_alive()
{
    _zn_transport_message_t msg;

    msg.header = _ZN_MID_KEEP_ALIVE;

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_copy_keep_alive(_zn_keep_alive_t *clone, _zn_keep_alive_t *keep_alive)
{
}

void _zn_t_msg_clear_keep_alive(_zn_keep_alive_t *msg, uint8_t header)
{
}

/*------------------ PingPong Messages ------------------*/
_zn_transport_message_t _zn_t_msg_make_ping(z_zint_t hash)
{
    _zn_transport_message_t msg;

    msg.body.ping_pong.hash = hash;

    msg.header = _ZN_MID_PING_PONG;

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_pong(z_zint_t hash)
{
    _zn_transport_message_t msg;

    msg.body.ping_pong.hash = hash;

    msg.header = _ZN_MID_PING_PONG;
    _ZN_SET_FLAG(msg.header, _ZN_FLAG_T_P);

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_clear_ping_pong(_zn_ping_pong_t *msg, uint8_t header)
{
    // NOTE: ping_pong does not involve any heap allocation
    (void)(header);
    (void)(msg);
}

/*------------------ Frame Message ------------------*/
_zn_transport_message_t _zn_t_msg_make_frame_header(z_zint_t sn, int is_reliable, int is_fragment, int is_final)
{
    _zn_transport_message_t msg;

    msg.body.frame.sn = sn;

    // Reset payload content
    memset(&msg.body.frame.payload, 0, sizeof(_zn_frame_payload_t));

    msg.header = _ZN_MID_FRAME;
    if (is_reliable)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_T_R);
    if (is_fragment)
    {
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_T_F);
        if (is_final)
            _ZN_SET_FLAG(msg.header, _ZN_FLAG_T_E);
    }

    msg.attachment = NULL;

    return msg;
}

_zn_transport_message_t _zn_t_msg_make_frame(z_zint_t sn, _zn_frame_payload_t payload, int is_reliable, int is_fragment, int is_final)
{
    _zn_transport_message_t msg;

    msg.body.frame.sn = sn;
    msg.body.frame.payload = payload;

    msg.header = _ZN_MID_FRAME;
    if (is_reliable)
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_T_R);
    if (is_fragment)
    {
        _ZN_SET_FLAG(msg.header, _ZN_FLAG_T_F);
        if (is_final)
            _ZN_SET_FLAG(msg.header, _ZN_FLAG_T_E);
    }

    msg.attachment = NULL;

    return msg;
}

void _zn_t_msg_clear_frame(_zn_frame_t *msg, uint8_t header)
{
    if (_ZN_HAS_FLAG(header, _ZN_FLAG_T_F))
        _zn_payload_clear(&msg->payload.fragment);
    else
        _zn_zenoh_message_vec_clear(&msg->payload.messages);
}

/*------------------ Transport Message ------------------*/
void _zn_t_msg_copy(_zn_transport_message_t *clone, _zn_transport_message_t *msg)
{
    clone->header = msg->header;
    clone->attachment = msg->attachment;

    uint8_t mid = _ZN_MID(msg->header);
    switch (mid)
    {
    case _ZN_MID_SCOUT:
        // _zn_t_msg_copy_scout(&clone->body.scout, &msg->body.scout);
        break;
    case _ZN_MID_HELLO:
        // _zn_t_msg_copy_hello(&clone->body.hello, &msg->body.hello);
        break;
    case _ZN_MID_JOIN:
        _zn_t_msg_copy_join(&clone->body.join, &msg->body.join);
        break;
    case _ZN_MID_INIT:
        _zn_t_msg_copy_init(&clone->body.init, &msg->body.init);
        break;
    case _ZN_MID_OPEN:
        _zn_t_msg_copy_open(&clone->body.open, &msg->body.open);
        break;
    case _ZN_MID_CLOSE:
        // _zn_t_msg_copy_close(&clone->body.close, &msg->body.close);
        break;
    case _ZN_MID_SYNC:
        // _zn_t_msg_copy_sync(&clone->body.sync, (&msg->body.sync);
        break;
    case _ZN_MID_ACK_NACK:
        // _zn_t_msg_copy_ack_nack(&clone->body.ack_nack, g->body.ack_nack);
        break;
    case _ZN_MID_KEEP_ALIVE:
        _zn_t_msg_copy_keep_alive(&clone->body.keep_alive, &msg->body.keep_alive);
        break;
    case _ZN_MID_PING_PONG:
        // _zn_t_msg_copy_ping_pong(&clone->body.ping_pong, ->body.ping_pong);
        break;
    case _ZN_MID_FRAME:
        // _zn_t_msg_copy_frame(&clone->body.frame, &msg->body.frame);
        break;
    default:
        _Z_ERROR("WARNING: Trying to free session message with unknown ID(%d)\n", mid);
        break;
    }
}

void _zn_t_msg_clear(_zn_transport_message_t *msg)
{
    if (msg->attachment)
    {
        _zn_t_msg_clear_attachment(msg->attachment);
        free(msg->attachment);
    }

    uint8_t mid = _ZN_MID(msg->header);
    switch (mid)
    {
    case _ZN_MID_SCOUT:
        _zn_t_msg_clear_scout(&msg->body.scout, msg->header);
        break;
    case _ZN_MID_HELLO:
        _zn_t_msg_clear_hello(&msg->body.hello, msg->header);
        break;
    case _ZN_MID_JOIN:
        _zn_t_msg_clear_join(&msg->body.join, msg->header);
        break;
    case _ZN_MID_INIT:
        _zn_t_msg_clear_init(&msg->body.init, msg->header);
        break;
    case _ZN_MID_OPEN:
        _zn_t_msg_clear_open(&msg->body.open, msg->header);
        break;
    case _ZN_MID_CLOSE:
        _zn_t_msg_clear_close(&msg->body.close, msg->header);
        break;
    case _ZN_MID_SYNC:
        _zn_t_msg_clear_sync(&msg->body.sync, msg->header);
        break;
    case _ZN_MID_ACK_NACK:
        _zn_t_msg_clear_ack_nack(&msg->body.ack_nack, msg->header);
        break;
    case _ZN_MID_KEEP_ALIVE:
        _zn_t_msg_clear_keep_alive(&msg->body.keep_alive, msg->header);
        break;
    case _ZN_MID_PING_PONG:
        _zn_t_msg_clear_ping_pong(&msg->body.ping_pong, msg->header);
        break;
    case _ZN_MID_FRAME:
        _zn_t_msg_clear_frame(&msg->body.frame, msg->header);
        return;
    default:
        _Z_ERROR("WARNING: Trying to free session message with unknown ID(%d)\n", mid);
        return;
    }
}
