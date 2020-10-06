/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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

#include <stdio.h>
#include <stdatomic.h>
#include "zenoh/private/logging.h"
#include "zenoh/net/recv_loop.h"
#include "zenoh/net/types.h"
#include "zenoh/net/private/net.h"
#include "zenoh/net/private/msgcodec.h"

typedef struct
{
    zn_session_t *z;
    z_zint_t qid;
    z_uint8_array_t qpid;
    atomic_int nb_qhandlers;
    atomic_flag sent_final;
} query_handle_t;

// void send_replies(void *query_handle, zn_resource_p_array_t replies, uint8_t eval_flag)
// {
//     unsigned int i;
//     int rsn = 0;
//     query_handle_t *handle = (query_handle_t *)query_handle;
//     _zn_message_t msg;
//     msg.header = _ZN_REPLY | _ZN_F_FLAG | eval_flag;
//     msg.payload.reply.qid = handle->qid;
//     msg.payload.reply.qpid = handle->qpid;
//     msg.payload.reply.srcid = handle->z->pid;

//     for (i = 0; i < replies.length; ++i)
//     {
//         msg.payload.reply.rsn = rsn++;
//         msg.payload.reply.rname = (char *)replies.elem[i]->rname;
//         _Z_DEBUG_VA("[%d] - Query reply key: %s\n", i, msg.payload.reply.rname);
//         _zn_payload_header_t ph;
//         ph.flags = _ZN_ENCODING | _ZN_KIND;
//         ph.encoding = replies.elem[i]->encoding;
//         ph.kind = replies.elem[i]->kind;
//         _Z_DEBUG_VA("[%d] - Payload Length: %zu\n", i, replies.elem[i]->length);
//         _Z_DEBUG_VA("[%d] - Payload address: %p\n", i, (void *)replies.elem[i]->data);

//         ph.payload = z_iobuf_wrap_wo((unsigned char *)replies.elem[i]->data, replies.elem[i]->length, 0, replies.elem[i]->length);
//         z_iobuf_t buf = z_iobuf_make(replies.elem[i]->length + 32);
//         _zn_payload_header_encode(&buf, &ph);
//         msg.payload.reply.payload_header = buf;

//         if (_zn_send_large_msg(handle->z->sock, &handle->z->wbuf, &msg, replies.elem[i]->length + 128) == 0)
//         {
//             z_iobuf_free(&buf);
//         }
//         else
//         {
//             _Z_DEBUG("Trying to reconnect....\n");
//             handle->z->on_disconnect(handle->z);
//             _zn_send_large_msg(handle->z->sock, &handle->z->wbuf, &msg, replies.elem[i]->length + 128);
//             z_iobuf_free(&buf);
//         }
//     }
//     msg.payload.reply.rsn = rsn++;
//     msg.payload.reply.rname = "";
//     z_iobuf_t buf = z_iobuf_make(0);
//     msg.payload.reply.payload_header = buf;

//     if (_zn_send_msg(handle->z->sock, &handle->z->wbuf, &msg) == 0)
//     {
//         z_iobuf_free(&buf);
//     }
//     else
//     {
//         _Z_DEBUG("Trying to reconnect....\n");
//         handle->z->on_disconnect(handle->z);
//         _zn_send_msg(handle->z->sock, &handle->z->wbuf, &msg);
//         z_iobuf_free(&buf);
//     }

//     atomic_fetch_sub(&handle->nb_qhandlers, 1);
//     if (handle->nb_qhandlers <= 0 && !atomic_flag_test_and_set(&handle->sent_final))
//     {
//         msg.header = _ZN_REPLY;

//         if (_zn_send_msg(handle->z->sock, &handle->z->wbuf, &msg) != 0)
//         {
//             _Z_DEBUG("Trying to reconnect....\n");
//             handle->z->on_disconnect(handle->z);
//             _zn_send_msg(handle->z->sock, &handle->z->wbuf, &msg);
//         }
//     }
// }

// void send_eval_replies(void *query_handle, zn_resource_p_array_t replies)
// {
//     send_replies(query_handle, replies, _ZN_E_FLAG);
// }

// void send_storage_replies(void *query_handle, zn_resource_p_array_t replies)
// {
//     send_replies(query_handle, replies, 0);
// }

// void handle_msg(zn_session_t *z, _zn_message_p_result_t r)
// {
//     _zn_payload_header_result_t r_ph;
//     zn_data_info_t info;
//     _zn_declaration_t *decls;
//     zn_res_key_t rkey;
//     const char *rname;
//     uint8_t mid;
//     z_list_t *subs;
//     z_list_t *stos;
//     z_list_t *evals;
//     z_list_t *lit;
//     _zn_sub_t *sub;
//     _zn_sto_t *sto;
//     _zn_eva_t *eval;
//     _zn_replywaiter_t *rw;
//     zn_reply_value_t rvalue;
//     _zn_res_decl_t *rd;
//     unsigned int i;
//     rname = 0;
//     subs = z_list_empty;
//     stos = z_list_empty;
//     lit = z_list_empty;
//     mid = _ZN_MID(r.value.message->header);
//     switch (mid)
//     {
//     case _ZN_STREAM_DATA:
//         _Z_DEBUG_VA("Received _Z_STREAM_DATA message %d\n", _Z_MID(r.value.message->header));
//         rname = _zn_get_resource_name(z, r.value.message->payload.stream_data.rid);
//         if (rname != 0)
//         {
//             subs = _zn_get_subscriptions_by_rname(z, rname);
//             stos = _zn_get_storages_by_rname(z, rname);
//             rkey.kind = ZN_STR_RES_KEY;
//             rkey.key.rname = (char *)rname;
//         }
//         else
//         {
//             subs = _zn_get_subscriptions_by_rid(z, r.value.message->payload.stream_data.rid);
//             rkey.kind = ZN_INT_RES_KEY;
//             rkey.key.rid = r.value.message->payload.stream_data.rid;
//         }

//         if (subs != 0 || stos != 0)
//         {
//             _zn_payload_header_decode_na(&r.value.message->payload.stream_data.payload_header, &r_ph);
//             if (r_ph.tag == Z_OK_TAG)
//             {
//                 info.flags = r_ph.value.payload_header.flags;
//                 info.encoding = r_ph.value.payload_header.encoding;
//                 info.kind = r_ph.value.payload_header.kind;
//                 if (info.flags & _ZN_T_STAMP)
//                 {
//                     info.tstamp = r_ph.value.payload_header.tstamp;
//                 }
//                 lit = subs;
//                 while (lit != z_list_empty)
//                 {
//                     sub = z_list_head(lit);
//                     sub->data_handler(&rkey, r_ph.value.payload_header.payload.buf, z_iobuf_readable(&r_ph.value.payload_header.payload), &info, sub->arg);
//                     lit = z_list_tail(lit);
//                 }
//                 lit = stos;
//                 while (lit != z_list_empty)
//                 {
//                     sto = z_list_head(lit);
//                     sto->data_handler(&rkey, r_ph.value.payload_header.payload.buf, z_iobuf_readable(&r_ph.value.payload_header.payload), &info, sto->arg);
//                     lit = z_list_tail(lit);
//                 }
//                 free(r_ph.value.payload_header.payload.buf);
//                 z_list_free(&subs);
//                 z_list_free(&stos);
//             }
//             else
//             {
//                 do
//                 {
//                     _Z_DEBUG("Unable to parse StreamData Message Payload Header\n");
//                 } while (0);
//             }
//         }
//         else
//         {
//             _Z_DEBUG_VA("No subscription found for resource %zu\n", r.value.message->payload.stream_data.rid);
//         }
//         z_iobuf_free(&r.value.message->payload.stream_data.payload_header);
//         break;
//     case _ZN_COMPACT_DATA:
//         _Z_DEBUG_VA("Received _Z_COMPACT_DATA message %d\n", _Z_MID(r.value.message->header));
//         rname = _zn_get_resource_name(z, r.value.message->payload.stream_data.rid);
//         if (rname != 0)
//         {
//             subs = _zn_get_subscriptions_by_rname(z, rname);
//             stos = _zn_get_storages_by_rname(z, rname);
//             rkey.kind = ZN_STR_RES_KEY;
//             rkey.key.rname = (char *)rname;
//         }
//         else
//         {
//             subs = _zn_get_subscriptions_by_rid(z, r.value.message->payload.stream_data.rid);
//             rkey.kind = ZN_INT_RES_KEY;
//             rkey.key.rid = r.value.message->payload.stream_data.rid;
//         }

//         if (subs != 0 || stos != 0)
//         {
//             info.flags = 0;
//             lit = subs;
//             while (lit != z_list_empty)
//             {
//                 sub = z_list_head(lit);
//                 sub->data_handler(
//                     &rkey,
//                     r.value.message->payload.compact_data.payload.buf,
//                     z_iobuf_readable(&r.value.message->payload.compact_data.payload),
//                     &info,
//                     sub->arg);
//                 lit = z_list_tail(lit);
//             }
//             lit = stos;
//             while (lit != z_list_empty)
//             {
//                 sto = z_list_head(lit);
//                 sto->data_handler(
//                     &rkey,
//                     r.value.message->payload.compact_data.payload.buf,
//                     z_iobuf_readable(&r.value.message->payload.compact_data.payload),
//                     &info,
//                     sto->arg);
//                 lit = z_list_tail(lit);
//             }
//             free(r.value.message->payload.compact_data.payload.buf);
//             z_list_free(&subs);
//         }
//         break;
//     case _ZN_WRITE_DATA:
//         _Z_DEBUG_VA("Received _Z_WRITE_DATA message %d\n", _Z_MID(r.value.message->header));
//         subs = _zn_get_subscriptions_by_rname(z, r.value.message->payload.write_data.rname);
//         stos = _zn_get_storages_by_rname(z, r.value.message->payload.write_data.rname);
//         if (subs != 0 || stos != 0)
//         {
//             rkey.kind = ZN_STR_RES_KEY;
//             rkey.key.rname = r.value.message->payload.write_data.rname;
//             _Z_DEBUG("Decoding Payload Header");
//             _zn_payload_header_decode_na(&r.value.message->payload.write_data.payload_header, &r_ph);
//             if (r_ph.tag == Z_OK_TAG)
//             {
//                 info.flags = r_ph.value.payload_header.flags;
//                 info.encoding = r_ph.value.payload_header.encoding;
//                 info.kind = r_ph.value.payload_header.kind;
//                 if (info.flags & _ZN_T_STAMP)
//                 {
//                     info.tstamp = r_ph.value.payload_header.tstamp;
//                 }
//                 while (subs != z_list_empty)
//                 {
//                     sub = (_zn_sub_t *)z_list_head(subs);
//                     sub->data_handler(
//                         &rkey,
//                         r_ph.value.payload_header.payload.buf,
//                         z_iobuf_readable(&r_ph.value.payload_header.payload),
//                         &info,
//                         sub->arg);
//                     subs = z_list_tail(subs);
//                 }
//                 while (stos != z_list_empty)
//                 {
//                     sto = (_zn_sto_t *)z_list_head(stos);
//                     sto->data_handler(
//                         &rkey,
//                         r_ph.value.payload_header.payload.buf,
//                         z_iobuf_readable(&r_ph.value.payload_header.payload),
//                         &info,
//                         sto->arg);
//                     stos = z_list_tail(stos);
//                 }
//                 free(r_ph.value.payload_header.payload.buf);
//             }
//             else
//                 _Z_DEBUG("Unable to parse WriteData Message Payload Header\n");
//         }
//         else
//         {
//             _Z_DEBUG_VA("No subscription found for resource %s\n", r.value.message->payload.write_data.rname);
//         }
//         z_iobuf_free(&r.value.message->payload.write_data.payload_header);
//         break;
//     case _ZN_QUERY:
//         _Z_DEBUG("Received _Z_QUERY message\n");
//         stos = _zn_get_storages_by_rname(z, r.value.message->payload.query.rname);
//         evals = _zn_get_evals_by_rname(z, r.value.message->payload.query.rname);
//         if (stos != 0 || evals != 0)
//         {
//             query_handle_t *query_handle = malloc(sizeof(query_handle_t));
//             query_handle->z = z;
//             query_handle->qid = r.value.message->payload.query.qid;
//             query_handle->qpid = r.value.message->payload.query.pid;

//             int nb_qhandlers = 0;
//             if (stos != 0)
//             {
//                 nb_qhandlers += z_list_len(stos);
//             }
//             if (evals != 0)
//             {
//                 nb_qhandlers += z_list_len(evals);
//             }
//             atomic_init(&query_handle->nb_qhandlers, nb_qhandlers);
//             atomic_flag_clear(&query_handle->sent_final);

//             if (stos != 0)
//             {
//                 lit = stos;
//                 while (lit != z_list_empty)
//                 {
//                     sto = (_zn_sto_t *)z_list_head(lit);
//                     sto->query_handler(
//                         r.value.message->payload.query.rname,
//                         r.value.message->payload.query.predicate,
//                         send_storage_replies,
//                         query_handle,
//                         sto->arg);
//                     lit = z_list_tail(lit);
//                 }
//                 z_list_free(&stos);
//             }

//             if (evals != 0)
//             {
//                 lit = evals;
//                 while (lit != z_list_empty)
//                 {
//                     eval = (_zn_eva_t *)z_list_head(lit);
//                     eval->query_handler(
//                         r.value.message->payload.query.rname,
//                         r.value.message->payload.query.predicate,
//                         send_eval_replies,
//                         query_handle,
//                         eval->arg);
//                     lit = z_list_tail(lit);
//                 }
//                 z_list_free(&evals);
//             }
//         }
//         break;
//     case _ZN_REPLY:
//         _Z_DEBUG("Received _Z_REPLY message\n");
//         rw = _zn_get_query(z, r.value.message->payload.reply.qid);
//         if (rw != 0)
//         {
//             if (r.value.message->header & _ZN_F_FLAG)
//             {
//                 rvalue.srcid = r.value.message->payload.reply.srcid.elem;
//                 rvalue.srcid_length = r.value.message->payload.reply.srcid.length;
//                 rvalue.rsn = r.value.message->payload.reply.rsn;
//                 if (strlen(r.value.message->payload.reply.rname) != 0)
//                 {
//                     rvalue.rname = r.value.message->payload.reply.rname;
//                     _zn_payload_header_decode_na(&r.value.message->payload.reply.payload_header, &r_ph);
//                     if (r_ph.tag == Z_OK_TAG)
//                     {
//                         rvalue.info.flags = r_ph.value.payload_header.flags;
//                         rvalue.info.encoding = r_ph.value.payload_header.encoding;
//                         rvalue.info.kind = r_ph.value.payload_header.kind;
//                         if (rvalue.info.flags & _ZN_T_STAMP)
//                         {
//                             rvalue.info.tstamp = r_ph.value.payload_header.tstamp;
//                         }
//                         rvalue.data = r_ph.value.payload_header.payload.buf;
//                         rvalue.data_length = z_iobuf_readable(&r_ph.value.payload_header.payload);
//                     }
//                     else
//                     {
//                         _Z_DEBUG("Unable to parse Reply Message Payload Header\n");
//                         break;
//                     }
//                     if (r.value.message->header & _ZN_E_FLAG)
//                     {
//                         rvalue.kind = ZN_EVAL_DATA;
//                     }
//                     else
//                     {
//                         rvalue.kind = ZN_STORAGE_DATA;
//                     }
//                 }
//                 else
//                 {
//                     if (r.value.message->header & _ZN_E_FLAG)
//                     {
//                         rvalue.kind = ZN_EVAL_FINAL;
//                     }
//                     else
//                     {
//                         rvalue.kind = ZN_STORAGE_FINAL;
//                     }
//                 }
//             }
//             else
//             {
//                 rvalue.kind = ZN_REPLY_FINAL;
//             }
//             rw->reply_handler(&rvalue, rw->arg);

//             switch (rvalue.kind)
//             {
//             case ZN_STORAGE_DATA:
//                 free((void *)rvalue.data);
//                 free((void *)rvalue.rname);
//                 free((void *)rvalue.srcid);
//                 break;
//             case ZN_STORAGE_FINAL:
//                 free((void *)rvalue.srcid);
//                 break;
//             case ZN_REPLY_FINAL:
//                 break;
//             default:
//                 break;
//             }
//         }
//         break;
//     case _ZN_DECLARE:
//         _Z_DEBUG("Received _ZN_DECLARE message\n");
//         decls = r.value.message->payload.declare.declarations.elem;
//         for (i = 0; i < r.value.message->payload.declare.declarations.length; ++i)
//         {
//             switch (_ZN_MID(decls[i].header))
//             {
//             case _ZN_RESOURCE_DECL:
//                 _Z_DEBUG("Received declare-resource message\n");
//                 _zn_register_res_decl(z, decls[i].payload.resource.rid, decls[i].payload.resource.r_name);
//                 break;
//             case _ZN_PUBLISHER_DECL:
//                 break;
//             case _ZN_SUBSCRIBER_DECL:
//                 _Z_DEBUG_VA("Registering remote subscription for resource: %zu\n", decls[i].payload.sub.rid);
//                 rd = _zn_get_res_decl_by_rid(z, decls[i].payload.sub.rid);
//                 if (rd != 0)
//                     z_i_map_set(z->remote_subs, decls[i].payload.sub.rid, rd);
//                 break;
//             case _ZN_COMMIT_DECL:
//                 break;

//             case _ZN_RESULT_DECL:
//                 break;
//             case _ZN_FORGET_RESOURCE_DECL:
//             case _ZN_FORGET_PUBLISHER_DECL:
//             case _ZN_FORGET_SUBSCRIBER_DECL:
//             case _ZN_FORGET_SELECTION_DECL:
//             case _ZN_STORAGE_DECL:
//             case _ZN_FORGET_STORAGE_DECL:
//             case _ZN_EVAL_DECL:
//             case _ZN_FORGET_EVAL_DECL:
//             default:
//                 break;
//             }
//         }
//         break;
//     case _ZN_ACCEPT:
//         _Z_DEBUG("Received _ZN_ACCEPT message\n");
//         break;
//     case _ZN_CLOSE:
//         _Z_DEBUG("Received _ZN_CLOSE message\n");
//         break;
//     default:
//         break;
//     }
// }

void handle_zenoh_msg(zn_session_t *z, _zn_zenoh_message_t *msg)
{
    if (z->running == 0)
    {
        return;
    }

    z_list_t *subs;
    _zn_sub_t *sub;
    subs = z_list_empty;

    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_DECLARE:
        _Z_DEBUG("Received _ZN_DECLARE message\n");
        for (unsigned int i = 0; i < msg->body.declare.declarations.length; ++i)
        {
            switch (_ZN_MID(msg->body.declare.declarations.elem[i].header))
            {
            case _ZN_DECL_RESOURCE:
                _Z_DEBUG("Received declare-resource message\n");
                printf("Received declare: %zu, %zu, %s\n", msg->body.declare.declarations.elem[i].body.res.rid, msg->body.declare.declarations.elem[i].body.res.key.rid, msg->body.declare.declarations.elem[i].body.res.key.rname);
                _zn_register_res_decl(z,
                                      msg->body.declare.declarations.elem[i].body.res.rid,
                                      msg->body.declare.declarations.elem[i].body.res.key.rname);
                break;
            case _ZN_DECL_PUBLISHER:
                break;
            case _ZN_DECL_SUBSCRIBER:
                // _Z_DEBUG_VA("Registering remote subscription for resource: %zu\n", decls[i].payload.sub.rid);
                // rd = _zn_get_res_decl_by_rid(z, decls[i].payload.sub.rid);
                // if (rd != 0)
                //     z_i_map_set(z->remote_subs, decls[i].payload.sub.rid, rd);
                break;
            case _ZN_DECL_QUERYABLE:
                break;
            case _ZN_DECL_FORGET_RESOURCE:
            case _ZN_DECL_FORGET_PUBLISHER:
            case _ZN_DECL_FORGET_SUBSCRIBER:
            case _ZN_DECL_FORGET_QUERYABLE:
            default:
                break;
            }
        }
        break;
    case _ZN_MID_DATA:
        _Z_DEBUG_VA("Received _ZN_MID_DATA message %d\n", _Z_MID(msg->header));
        if (msg->body.data.key.rid == ZN_NO_RESOURCE_ID)
        {
            if (msg->body.data.key.rname)
            {
                printf("RID: %s\n", msg->body.data.key.rname);
                subs = _zn_get_subscriptions_by_rname(z, msg->body.data.key.rname);

                while (subs && subs != z_list_empty)
                {
                    sub = (_zn_sub_t *)z_list_head(subs);
                    sub->data_handler(
                        &msg->body.data.key,
                        msg->body.data.payload.iobuf.buf,
                        z_iobuf_readable(&msg->body.data.payload.iobuf),
                        &msg->body.data.info,
                        sub->arg);
                    subs = z_list_tail(subs);
                }
            }
            else
            {
                _Z_DEBUG("Unknown zenoh message ID");
            }
        }
        else
        {
            subs = _zn_get_subscriptions_by_rid(z, msg->body.data.key.rid);

            while (subs != z_list_empty)
            {
                printf("RID: %zu\n", msg->body.data.key.rid);
                sub = (_zn_sub_t *)z_list_head(subs);
                sub->data_handler(
                    &msg->body.data.key,
                    msg->body.data.payload.iobuf.buf,
                    z_iobuf_readable(&msg->body.data.payload.iobuf),
                    &msg->body.data.info,
                    sub->arg);
                subs = z_list_tail(subs);
            }
        }

        break;
    case _ZN_MID_PULL:
        printf("Received pull");
        break;
    case _ZN_MID_QUERY:
        printf("Received query");
        break;
    case _ZN_MID_UNIT:
        printf("Received unit");
        // Do nothing. Unit messages have no body
        break;
    default:
        _Z_DEBUG("Unknown zenoh message ID");
        break;
    }
}

int handle_session_msg(zn_session_t *z, _zn_session_message_t *msg)
{
    if (z->running == 0)
    {
        return -1;
    }

    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_SCOUT:
        _Z_DEBUG("Handling of Scout messages not implemented");
        break;
    case _ZN_MID_HELLO:
        _Z_DEBUG("Handling of Hello messages not implemented");
        break;
    case _ZN_MID_OPEN:
        _Z_DEBUG("Handling of Open messages not implemented");
        break;
    case _ZN_MID_ACCEPT:
        _Z_DEBUG("Handling of Accept messages not implemented");
        break;
    case _ZN_MID_CLOSE:
        _Z_DEBUG("Handling of Close messages not implemented");
        break;
    case _ZN_MID_SYNC:
        _Z_DEBUG("Handling of Sync messages not implemented");
        break;
    case _ZN_MID_ACK_NACK:
        _Z_DEBUG("Handling of AckNack messages not implemented");
        break;
    case _ZN_MID_KEEP_ALIVE:
        _Z_DEBUG("Handling of KeepAlive messages not implemented");
        break;
    case _ZN_MID_PING_PONG:
        _Z_DEBUG("Handling of PingPong messages not implemented");
        break;
    case _ZN_MID_FRAME:
        if (!_ZN_HAS_FLAG(msg->header, _ZN_FLAG_S_F))
        {
            unsigned int len = z_vec_length(&msg->body.frame.payload.messages);
            for (unsigned int i = 0; i < len; ++i)
                handle_zenoh_msg(z, (_zn_zenoh_message_t *)z_vec_get(&msg->body.frame.payload.messages, i));
        }
        else
        {
            _Z_DEBUG("Handling of Fragmented Frame messages not implemented");
        }
        break;
    default:
        _Z_DEBUG("Unknown session message ID");
        return -1;
    }

    return 0;
}

void *zn_recv_loop(zn_session_t *z)
{
    z->running = 1;

    _zn_session_message_p_result_t r;
    _zn_session_message_p_result_init(&r);

    z_iobuf_clear(&z->rbuf);
    while (z->running)
    {
        z_iobuf_compact(&z->rbuf);

#ifdef ZENOH_NET_TRANSPORT_TCP_IP
        // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
        //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
        //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
        //       the boundary of the serialized messages. The length is encoded as little-endian.
        //       In any case, the length of a message must not exceed 65_535 bytes.

        // Read number of bytes to read
        while (z_iobuf_readable(&z->rbuf) < _ZN_MSG_LEN_ENC_SIZE)
        {
            if (_zn_recv_buf(z->sock, &z->rbuf) < 0)
                return 0;
        }

        // Decode the message length
        unsigned int to_read = (unsigned int)((uint16_t)z_iobuf_read(&z->rbuf) | ((uint16_t)z_iobuf_read(&z->rbuf) << 8));

        // Read the rest of bytes to decode one or more session messages.
        while (z_iobuf_readable(&z->rbuf) < to_read)
        {
            if (_zn_recv_buf(z->sock, &z->rbuf) < 0)
                return 0;
        }

        // Wrap the main buffer for to_read bytes
        z_iobuf_t rbuf = z_iobuf_wrap_wo((uint8_t *)(z->rbuf.buf + z->rbuf.r_pos), to_read, 0, to_read);
#else
        // Read bytes from the socket.
        while (z_iobuf_readable(&z->rbuf) == 0)
        {
            if (_zn_recv_buf(z->sock, &z->rbuf) < 0)
                return 0;
        }

        z_iobuf_t rbuf = z->rbuf;
#endif

        while (z_iobuf_readable(&rbuf) > 0)
        {
            // Decode one session message
            _zn_session_message_decode_na(&rbuf, &r);

            if (r.tag == Z_OK_TAG)
            {
                handle_session_msg(z, r.value.session_message);
            }
            else
            {
                _Z_DEBUG("Connection closed due to receive error");
                return 0;
            }
        }

#ifdef ZENOH_NET_TRANSPORT_TCP_IP
        // Move the read position of the read buffer
        z->rbuf.r_pos += to_read;
#endif
    }

    return 0;
}

int zn_running(zn_session_t *z)
{
    return z->running;
}
