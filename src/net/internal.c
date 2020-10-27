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

#include "zenoh/net/private/internal.h"
#include "zenoh/net/private/msg.h"
#include "zenoh/net/private/msgcodec.h"
#include "zenoh/net/private/net.h"
#include "zenoh/net/private/sync.h"
#include "zenoh/private/logging.h"
#include "zenoh/rname.h"

/*------------------ Transmission and Reception helper ------------------*/
void _zn_prepare_wbuf(z_wbuf_t *buf)
{
    z_wbuf_clear(buf);

#ifdef ZENOH_NET_TRANSPORT_TCP_IP
    // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
    //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
    //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
    //       the boundary of the serialized messages. The length is encoded as little-endian.
    //       In any case, the length of a message must not exceed 65_535 bytes.
    for (size_t i = 0; i < _ZN_MSG_LEN_ENC_SIZE; ++i)
        z_wbuf_put(buf, 0, i);

    z_wbuf_set_wpos(buf, _ZN_MSG_LEN_ENC_SIZE);
#endif /* ZENOH_NET_TRANSPORT_TCP_IP */
}

void _zn_finalize_wbuf(z_wbuf_t *buf)
{
#ifdef ZENOH_NET_TRANSPORT_TCP_IP
    // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
    //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
    //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
    //       the boundary of the serialized messages. The length is encoded as little-endian.
    //       In any case, the length of a message must not exceed 65_535 bytes.
    size_t len = z_wbuf_len(buf) - _ZN_MSG_LEN_ENC_SIZE;
    for (size_t i = 0; i < _ZN_MSG_LEN_ENC_SIZE; ++i)
        z_wbuf_put(buf, (uint8_t)((len >> 8 * i) & 0xFF), i);
#endif /* ZENOH_NET_TRANSPORT_TCP_IP */
}

int _zn_send_s_msg(zn_session_t *z, _zn_session_message_t *s_msg)
{
    _Z_DEBUG(">> send session message\n");

    // Acquire the lock
    _zn_mutex_lock(&z->mutex_tx);
    // Prepare the buffer eventually reserving space for the message length
    _zn_prepare_wbuf(&z->wbuf);
    // Encode the session message
    if (_zn_session_message_encode(&z->wbuf, s_msg) != 0)
    {
        _Z_DEBUG("Dropping session message because it is too large");
        // Release the lock
        _zn_mutex_unlock(&z->mutex_tx);
        return -1;
    }
    // Write the message legnth in the reserved space if needed
    _zn_finalize_wbuf(&z->wbuf);
    // Send the wbuf on the socket
    int res = _zn_send_wbuf(z->sock, &z->wbuf);
    // Release the lock
    _zn_mutex_unlock(&z->mutex_tx);

    // Mark the session that we have transmitted data
    z->transmitted = 1;

    return res;
}

// _zn_session_message_t _zn_frame_header(zn_session_t *z, int is_reliable, int is_fragment, int is_final)
// {
//     // Create the frame session message that carries the zenoh message
//     _zn_session_message_t s_msg;
//     _ZN_INIT_S_MSG(s_msg)
//     s_msg.header = _ZN_MID_FRAME;
//     if (is_reliable)
//     {
//         _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_R);
//         s_msg.body.frame.sn = z->sn_tx_reliable;
//         // Update the sequence number in modulo operation
//         z->sn_tx_reliable = (z->sn_tx_reliable + 1) % z->sn_resolution;
//     }
//     else
//     {
//         s_msg.body.frame.sn = z->sn_tx_best_effort;
//         // Update the sequence number in modulo operation
//         z->sn_tx_best_effort = (z->sn_tx_best_effort + 1) % z->sn_resolution;
//     }

//     if (is_fragment)
//     {
//         _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_F);
//     }

//     if (is_final)
//     {
//         _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_E);
//     }

//     // Do not allocate the vector containing the messages
//     s_msg.body.frame.payload.messages._capacity = 0;
//     s_msg.body.frame.payload.messages._length = 0;
//     s_msg.body.frame.payload.messages._elem = NULL;

//     return s_msg;
// }

z_zint_t _zn_get_sn(zn_session_t *z, int is_reliable)
{
    size_t sn;
    // Get the sequence number and update it in modulo operation
    if (is_reliable)
    {
        sn = z->sn_tx_reliable;
        z->sn_tx_reliable = (z->sn_tx_reliable + 1) % z->sn_resolution;
    }
    else
    {
        sn = z->sn_tx_best_effort;
        z->sn_tx_best_effort = (z->sn_tx_best_effort + 1) % z->sn_resolution;
    }
    return sn;
}

_zn_session_message_t _zn_frame_header(int is_reliable, int is_fragment, int is_final, z_zint_t sn)
{
    // Create the frame session message that carries the zenoh message
    _zn_session_message_t s_msg;
    _ZN_INIT_S_MSG(s_msg)
    s_msg.header = _ZN_MID_FRAME;
    s_msg.body.frame.sn = sn;

    if (is_reliable)
        _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_R);

    if (is_fragment)
    {
        _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_F);

        if (is_final)
            _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_E);

        // Do not add the payload
        s_msg.body.frame.payload.fragment.length = 0;
        s_msg.body.frame.payload.fragment.elem = NULL;
    }
    else
    {
        // Do not allocate the vector containing the messages
        s_msg.body.frame.payload.messages._capacity = 0;
        s_msg.body.frame.payload.messages._length = 0;
        s_msg.body.frame.payload.messages._elem = NULL;
    }

    return s_msg;
}

int _zn_serialize_zenoh_fragment(z_wbuf_t *dst, z_wbuf_t *src, int is_reliable, size_t sn)
{
    // Assume first that this is not the final fragment
    int is_final = 0;
    do
    {
        // Mark the buffer for the writing operation
        size_t w_pos = z_wbuf_get_wpos(dst);
        // Get the frame header
        _zn_session_message_t f_hdr = _zn_frame_header(is_reliable, 1, is_final, sn);
        // Encode the frame header
        int res = _zn_session_message_encode(dst, &f_hdr);
        if (res == 0)
        {
            size_t space_left = z_wbuf_space_left(dst);
            size_t bytes_left = z_wbuf_len(src);
            // Check if it is really the final fragment
            if (!is_final && (bytes_left <= space_left))
            {
                // Revert the buffer
                z_wbuf_set_wpos(dst, w_pos);
                // It is really the finally fragment, reserialize the header
                is_final = 1;
                continue;
            }
            // Write the fragment
            size_t to_copy = bytes_left <= space_left ? bytes_left : space_left;
            return z_wbuf_copy_into(dst, src, to_copy);
        }
        else
        {
            return 0;
        }
    } while (1);
}

int _zn_send_z_msg(zn_session_t *z, _zn_zenoh_message_t *z_msg, int is_reliable)
{
    _Z_DEBUG(">> send zenoh message\n");

    // Acquire the lock
    _zn_mutex_lock(&z->mutex_tx);
    // Prepare the buffer eventually reserving space for the message length
    _zn_prepare_wbuf(&z->wbuf);

    // Get the next sequence number
    size_t sn = _zn_get_sn(z, is_reliable);
    // Create the frame header that carries the zenoh message
    _zn_session_message_t s_msg = _zn_frame_header(is_reliable, 0, 0, sn);

    // Encode the frame header
    int res = _zn_session_message_encode(&z->wbuf, &s_msg);
    if (res != 0)
    {
        _Z_DEBUG("Dropping zenoh message because the session frame can not be encoded");
        // Release the lock
        _zn_mutex_unlock(&z->mutex_tx);
        return -1;
    }

    // Encode the zenoh message
    res = _zn_zenoh_message_encode(&z->wbuf, z_msg);
    if (res == 0)
    {
        // Write the message legnth in the reserved space if needed
        _zn_finalize_wbuf(&z->wbuf);
        // Send the wbuf on the socket
        res = _zn_send_wbuf(z->sock, &z->wbuf);
        if (res == 0)
            // Mark the session that we have transmitted data
            z->transmitted = 1;
    }
    else
    {
        // The message does not fit in the current batch, let's fragment it
        // Create an expandable wbuf for fragmentation
        z_wbuf_t fbf = z_wbuf_make(ZENOH_NET_FRAG_BUF_TX_CHUNK, 1);

        // Encode the message on the expandable wbuf
        res = _zn_zenoh_message_encode(&fbf, z_msg);
        if (res != 0)
        {
            _Z_DEBUG("Dropping zenoh message because it can not be fragmented");
            goto EXIT_FRAG_PROC;
        }

        // Fragment and send the message
        int is_first = 1;
        while (z_wbuf_len(&fbf) > 0)
        {
            // Get the fragment sequence number
            if (!is_first)
                sn = _zn_get_sn(z, is_reliable);
            is_first = 0;

            // Clear the buffer for serialization
            _zn_prepare_wbuf(&z->wbuf);

            // Serialize one fragment
            res = _zn_serialize_zenoh_fragment(&z->wbuf, &fbf, is_reliable, sn);
            if (res != 0)
            {
                _Z_DEBUG("Dropping zenoh message because it can not be fragmented");
                goto EXIT_FRAG_PROC;
            }

            // Write the message length in the reserved space if needed
            _zn_finalize_wbuf(&z->wbuf);

            // Send the wbuf on the socket
            res = _zn_send_wbuf(z->sock, &z->wbuf);
            if (res != 0)
            {
                _Z_DEBUG("Dropping zenoh message because it can not sent");
                goto EXIT_FRAG_PROC;
            }

            // Mark the session that we have transmitted data
            z->transmitted = 1;
        }

    EXIT_FRAG_PROC:
        // Free the fragmentation buffer memory
        z_wbuf_free(&fbf);
    }

    // Release the lock
    _zn_mutex_unlock(&z->mutex_tx);

    return res;
}

void _zn_recv_s_msg_na(zn_session_t *z, _zn_session_message_p_result_t *r)
{
    _Z_DEBUG(">> recv session msg\n");
    r->tag = Z_OK_TAG;

    // Acquire the lock
    _zn_mutex_lock(&z->mutex_rx);
    // Prepare the buffer
    z_rbuf_clear(&z->rbuf);

#ifdef ZENOH_NET_TRANSPORT_TCP_IP
    // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
    //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
    //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
    //       the boundary of the serialized messages. The length is encoded as little-endian.
    //       In any case, the length of a message must not exceed 65_535 bytes.

    // Read the message length
    if (_zn_recv_bytes(z->sock, z->rbuf.ios.buf, _ZN_MSG_LEN_ENC_SIZE) < 0)
    {
        // Release the lock
        _zn_mutex_lock(&z->mutex_rx);
        _zn_session_message_p_result_free(r);
        r->tag = Z_ERROR_TAG;
        r->value.error = ZN_IO_ERROR;
        return;
    }
    z_rbuf_set_wpos(&z->rbuf, _ZN_MSG_LEN_ENC_SIZE);

    uint16_t len = z_rbuf_read(&z->rbuf) | (z_rbuf_read(&z->rbuf) << 8);
    _Z_DEBUG_VA(">> \t msg len = %zu\n", len);
    size_t writable = z_rbuf_capacity(&z->rbuf) - z_rbuf_len(&z->rbuf);
    if (writable < len)
    {
        // Release the lock
        _zn_mutex_unlock(&z->mutex_rx);
        _zn_session_message_p_result_free(r);
        r->tag = Z_ERROR_TAG;
        r->value.error = ZN_INSUFFICIENT_IOBUF_SIZE;
        return;
    }

    // Read enough bytes to decode the message
    if (_zn_recv_bytes(z->sock, z->rbuf.ios.buf, len) < 0)
    {
        // Release the lock
        _zn_mutex_unlock(&z->mutex_rx);
        _zn_session_message_p_result_free(r);
        r->tag = Z_ERROR_TAG;
        r->value.error = ZN_IO_ERROR;
        return;
    }

    z_rbuf_set_rpos(&z->rbuf, 0);
    z_rbuf_set_wpos(&z->rbuf, len);
#else
    if (_zn_recv_buf(sock, buf) < 0)
    {
        // Release the lock
        _zn_mutex_unlock(&z->mutex_rx);
        _zn_session_message_p_result_free(r);
        r->tag = Z_ERROR_TAG;
        r->value.error = ZN_IO_ERROR;
        return;
    }
#endif /* ZENOH_NET_TRANSPORT_TCP_IP */

    // Mark the session that we have received data
    z->received = 1;

    _Z_DEBUG(">> \t session_message_decode\n");
    _zn_session_message_decode_na(&z->rbuf, r);

    // Release the lock
    _zn_mutex_unlock(&z->mutex_rx);
}

_zn_session_message_p_result_t _zn_recv_s_msg(zn_session_t *z)
{
    _zn_session_message_p_result_t r;
    _zn_session_message_p_result_init(&r);
    _zn_recv_s_msg_na(z, &r);
    return r;
}

/*------------------ Entity ------------------*/
z_zint_t _zn_get_entity_id(zn_session_t *z)
{
    return z->entity_id++;
}

/*------------------ Resource ------------------*/
z_zint_t _zn_get_resource_id(zn_session_t *z, const zn_res_key_t *res_key)
{
    _zn_res_decl_t *res_decl = _zn_get_resource_by_key(z, _ZN_IS_LOCAL, res_key);
    if (res_decl)
    {
        return res_decl->id;
    }
    else
    {
        z_zint_t id = z->resource_id++;
        while (_zn_get_resource_by_id(z, _ZN_IS_LOCAL, id) != 0)
        {
            id++;
        }
        z->resource_id = id;
        return id;
    }
}

const char *_zn_get_resource_name_from_key(zn_session_t *z, int is_local, const zn_res_key_t *res_key)
{
    char *rname = NULL;

    z_list_t *resources = is_local ? z->local_resources : z->remote_resources;
    if (resources)
    {
        z_list_t *strs = z_list_empty;
        z_zint_t rid = res_key->rid;
        _zn_res_decl_t *decl = _zn_get_resource_by_id(z, is_local, rid);

        // Travel all the matching RID to reconstruct the full resource name
        size_t len = 0;
        while (decl)
        {
            len += strlen(decl->key.rname);
            strs = z_list_cons(strs, decl->key.rname);

            if (decl->id == ZN_NO_RESOURCE_ID)
                break;

            decl = _zn_get_resource_by_id(z, is_local, decl->key.rid);
        }

        if (strs)
        {
            // Concatenate all the partial resource names
            rname = (char *)malloc(len + 1);
            char *s = (char *)z_list_head(strs);
            while (strs)
            {
                strcat(rname, s);
                s = (char *)z_list_head(strs);
                strs = z_list_tail(strs);
            }
        }
    }

    return rname;
}

_zn_res_decl_t *_zn_get_resource_by_id(zn_session_t *z, int is_local, z_zint_t id)
{
    z_list_t *decls = is_local ? z->local_resources : z->remote_resources;
    _zn_res_decl_t *decl = NULL;
    while (decls)
    {
        decl = (_zn_res_decl_t *)z_list_head(decls);

        if (decl->id == id)
            break;

        decls = z_list_tail(decls);
    }

    return decl;
}

_zn_res_decl_t *_zn_get_resource_by_key(zn_session_t *z, int is_local, const zn_res_key_t *res_key)
{
    z_list_t *decls = is_local ? z->local_resources : z->remote_resources;
    _zn_res_decl_t *decl = NULL;
    while (decls)
    {
        decl = (_zn_res_decl_t *)z_list_head(decls);

        if (decl->key.rid == res_key->rid && strcmp(decl->key.rname, res_key->rname) == 0)
            break;

        decls = z_list_tail(decls);
    }

    return decl;
}

int _zn_register_resource(zn_session_t *z, int is_local, z_zint_t id, const zn_res_key_t *res_key)
{
    _Z_DEBUG_VA(">>> Allocating res decl for (%zu,%s)\n", res_key->rid, res_key->rname);
    _zn_res_decl_t *rd_rid = _zn_get_resource_by_id(z, is_local, id);
    _zn_res_decl_t *rd_key = _zn_get_resource_by_key(z, is_local, res_key);

    if (!rd_rid && !rd_key)
    {
        // No resource declaration has been found, create a new one
        _zn_res_decl_t *rdecl = (_zn_res_decl_t *)malloc(sizeof(_zn_res_decl_t));
        rdecl->id = id;
        rdecl->key.rid = res_key->rid;
        rdecl->key.rname = strdup(res_key->rname);

        if (is_local)
            z->local_resources = z_list_cons(z->local_resources, rdecl);
        else
            z->remote_resources = z_list_cons(z->remote_resources, rdecl);

        return 0;
    }
    else if (rd_rid == rd_key)
    {
        // A resource declaration for this id and key has been found, return
        return 1;
    }
    else
    {
        // Inconsistent declarations have been found, return an error
        return -1;
    }
}

int _zn_resource_predicate(void *elem, void *arg)
{
    _zn_res_decl_t *rel = (_zn_res_decl_t *)elem;
    _zn_res_decl_t *rar = (_zn_res_decl_t *)arg;
    if (rel->id == rar->id)
        return 1;
    else
        return 0;
}

void _zn_unregister_resource(zn_session_t *z, int is_local, _zn_res_decl_t *r)
{
    if (is_local)
        z->local_resources = z_list_remove(z->local_resources, _zn_resource_predicate, r);
    else
        z->remote_resources = z_list_remove(z->remote_resources, _zn_resource_predicate, r);
}

/*------------------ Subscription ------------------*/
z_list_t *_zn_get_subscriptions_from_remote_key(zn_session_t *z, const zn_res_key_t *res_key)
{
    z_list_t *xs = z_list_empty;

    // First try to get the remote id->local subscription mapping if numeric-only resources
    if (!res_key->rname)
    {
        _zn_sub_t *sub = (_zn_sub_t *)z_i_map_get(z->rem_res_loc_sub_map, res_key->rid);
        if (sub)
            return z_list_cons(xs, sub);
        else
            return xs;
    }

    // If no mapping was found, then try to get a remote resource declaration
    _zn_res_decl_t *remote = _zn_get_resource_by_id(z, _ZN_IS_REMOTE, res_key->rid);
    if (!remote)
    {
        // No remote resource was found matching the provided id, return empty list
        return xs;
    }

    // Check if there is a local subscriptions matching the remote resource declaration
    _zn_res_decl_t *local = _zn_get_resource_by_key(z, _ZN_IS_LOCAL, &remote->key);
    if (!local)
    {
        // No local resource was found matching the provided key, return empty list
        return xs;
    }

    z_list_t *subs = z->local_subscriptions;
    _zn_sub_t *sub;
    while (subs)
    {
        sub = (_zn_sub_t *)z_list_head(subs);

        // Check if the resource ids match
        if (sub->key.rid == local->key.rid)
        {
            if (sub->key.rname)
            {
                // A string resource has been defined, check if they intersect
                if (zn_rname_intersect(sub->key.rname, local->key.rname))
                    xs = z_list_cons(xs, sub);
            }
            else
            {
                xs = z_list_cons(xs, sub);
            }
        }

        subs = z_list_tail(subs);
    }

    return xs;
}

_zn_sub_t *_zn_get_subscription_by_id(zn_session_t *z, int is_local, z_zint_t id)
{
    z_list_t *subs = is_local ? z->local_subscriptions : z->remote_subscriptions;
    _zn_sub_t *sub;
    while (subs)
    {
        sub = (_zn_sub_t *)z_list_head(subs);

        if (sub->id == id)
            return sub;

        subs = z_list_tail(subs);
    }

    return NULL;
}

_zn_sub_t *_zn_get_subscription_by_key(zn_session_t *z, int is_local, const zn_res_key_t *res_key)
{
    z_list_t *subs = is_local ? z->local_subscriptions : z->remote_subscriptions;

    _zn_sub_t *sub;
    while (subs)
    {
        sub = (_zn_sub_t *)z_list_head(subs);

        if (sub->key.rid == res_key->rid && strcmp(sub->key.rname, res_key->rname) == 0)
            return sub;

        subs = z_list_tail(subs);
    }

    return NULL;
}

int _zn_register_subscription(zn_session_t *z, int is_local, _zn_sub_t *sub)
{
    _Z_DEBUG_VA(">>> Allocating sub decl for (%zu,%s)\n", res_key->rid, res_key->rname);

    _zn_sub_t *s = _zn_get_subscription_by_id(z, is_local, sub->id);
    if (s)
    {
        // A subscription for this id already exists, return
        return -1;
    }

    s = _zn_get_subscription_by_key(z, is_local, &sub->key);
    if (s)
    {
        // A subscription for this key already exists, return
        return -1;
    }

    // Register the new subscription
    s = (_zn_sub_t *)malloc(sizeof(_zn_sub_t));

    s->id = sub->id;
    s->key.rid = sub->key.rid;
    if (sub->key.rname)
        s->key.rname = strdup(sub->key.rname);
    else
        s->key.rname = NULL;
    memcpy(&s->info, &sub->info, sizeof(zn_sub_info_t));
    s->data_handler = sub->data_handler;
    s->arg = sub->arg;

    if (is_local)
        z->local_subscriptions = z_list_cons(z->local_subscriptions, s);
    else
        z->remote_subscriptions = z_list_cons(z->remote_subscriptions, s);

    return 0;
}

int _zn_subscription_predicate(void *elem, void *arg)
{
    _zn_sub_t *s = (_zn_sub_t *)arg;
    _zn_sub_t *sub = (_zn_sub_t *)elem;
    if (sub->id == s->id)
        return 1;
    else
        return 0;
}

void _zn_unregister_subscription(zn_session_t *z, int is_local, _zn_sub_t *s)
{
    if (is_local)
        z->local_subscriptions = z_list_remove(z->local_subscriptions, _zn_subscription_predicate, s);
    else
        z->remote_subscriptions = z_list_remove(z->remote_subscriptions, _zn_subscription_predicate, s);
}

/*------------------ Queryable ------------------*/
// void _zn_register_queryable(zn_session_t *z, z_zint_t rid, z_zint_t id, zn_query_handler_t query_handler, void *arg)
// {
//     _zn_qle_t *qle = (_zn_qle_t *)malloc(sizeof(_zn_qle_t));
//     qle->rid = rid;
//     qle->id = id;
//     _zn_res_decl_t *decl = _zn_get_res_decl_by_rid(z, rid);
//     assert(decl != 0);
//     qle->rname = strdup(decl->key.rname);
//     qle->query_handler = query_handler;
//     qle->arg = arg;
//     z->queryables = z_list_cons(z->queryables, qle);
// }

// int qle_predicate(void *elem, void *arg)
// {
//     zn_qle_t *q = (zn_qle_t *)arg;
//     _zn_qle_t *queryable = (_zn_qle_t *)elem;
//     if (queryable->id == q->id)
//     {
//         return 1;
//     }
//     else
//     {
//         return 0;
//     }
// }

// void _zn_unregister_queryable(zn_qle_t *e)
// {
//     e->z->queryables = z_list_remove(e->z->queryables, qle_predicate, e);
// }

// z_list_t *_zn_get_queryables_by_rid(zn_session_t *z, z_zint_t rid)
// {
//     z_list_t *queryables = z_list_empty;
//     if (z->queryables == 0)
//     {
//         return queryables;
//     }
//     else
//     {
//         _zn_qle_t *queryable = 0;
//         z_list_t *queryables = z->queryables;
//         z_list_t *xs = z_list_empty;
//         do
//         {
//             queryable = (_zn_qle_t *)z_list_head(queryables);
//             queryables = z_list_tail(queryables);
//             if (queryable->rid == rid)
//             {
//                 xs = z_list_cons(xs, queryable);
//             }
//         } while (queryables != 0);
//         return xs;
//     }
// }

// z_list_t *_zn_get_queryables_by_rname(zn_session_t *z, const char *rname)
// {
//     z_list_t *queryables = z_list_empty;
//     if (z->queryables == 0)
//     {
//         return queryables;
//     }
//     else
//     {
//         _zn_qle_t *queryable = 0;
//         z_list_t *queryables = z->queryables;
//         z_list_t *xs = z_list_empty;
//         do
//         {
//             queryable = (_zn_qle_t *)z_list_head(queryables);
//             queryables = z_list_tail(queryables);
//             if (zn_rname_intersect(queryable->rname, (char *)rname))
//             {
//                 xs = z_list_cons(xs, queryable);
//             }
//         } while (queryables != 0);
//         return xs;
//     }
// }

// int _zn_matching_remote_sub(zn_session_t *z, z_zint_t rid)
// {
//     return z_i_map_get(z->remote_subscriptions, rid) != 0 ? 1 : 0;
// }

// void _zn_register_query(zn_session_t *z, z_zint_t qid, zn_reply_handler_t reply_handler, void *arg)
// {
//     _zn_replywaiter_t *rw = (_zn_replywaiter_t *)malloc(sizeof(_zn_replywaiter_t));
//     rw->qid = qid;
//     rw->reply_handler = reply_handler;
//     rw->arg = arg;
//     z->replywaiters = z_list_cons(z->replywaiters, rw);
// }

// _zn_replywaiter_t *_zn_get_query(zn_session_t *z, z_zint_t qid)
// {
//     if (z->replywaiters == 0)
//     {
//         return 0;
//     }
//     else
//     {
//         _zn_replywaiter_t *rw = (_zn_replywaiter_t *)z_list_head(z->replywaiters);
//         z_list_t *rws = z_list_tail(z->replywaiters);
//         while (rws != 0 && rw->qid != qid)
//         {
//             rw = z_list_head(rws);
//             rws = z_list_tail(rws);
//         }
//         if (rw->qid == qid)
//             return rw;
//         else
//             return 0;
//     }
// }
