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
#include "zenoh/net/private/system.h"
#include "zenoh/net/rname.h"
#include "zenoh/private/logging.h"
#include "zenoh/utils.h"

/*------------------ ResKey helpers ------------------*/
zn_reskey_t _zn_reskey_clone(zn_reskey_t *reskey)
{
    zn_reskey_t rk;
    rk.rid = reskey->rid;
    if (reskey->rname)
        rk.rname = strdup(reskey->rname);
    else
        rk.rname = NULL;
    return rk;
}

/*------------------ Message helpers ------------------*/
_zn_session_message_t _zn_session_message_init(uint8_t header)
{
    _zn_session_message_t sm;
    memset(&sm, 0, sizeof(_zn_session_message_t));
    sm.header = header;
    return sm;
}

_zn_zenoh_message_t _zn_zenoh_message_init(uint8_t header)
{
    _zn_zenoh_message_t zm;
    memset(&zm, 0, sizeof(_zn_zenoh_message_t));
    zm.header = header;
    return zm;
}

/*------------------ SN helper ------------------*/
z_zint_t _zn_get_sn(zn_session_t *zn, zn_reliability_t reliability)
{
    size_t sn;
    // Get the sequence number and update it in modulo operation
    if (reliability == zn_reliability_t_RELIABLE)
    {
        sn = zn->sn_tx_reliable;
        zn->sn_tx_reliable = (zn->sn_tx_reliable + 1) % zn->sn_resolution;
    }
    else
    {
        sn = zn->sn_tx_best_effort;
        zn->sn_tx_best_effort = (zn->sn_tx_best_effort + 1) % zn->sn_resolution;
    }
    return sn;
}

int _zn_sn_precedes(z_zint_t sn_resolution_half, z_zint_t sn_left, z_zint_t sn_right)
{
    if (sn_right > sn_left)
        return (sn_right - sn_left <= sn_resolution_half);
    else
        return (sn_left - sn_right > sn_resolution_half);
}

/*------------------ Transmission and Reception helper ------------------*/
void _zn_prepare_wbuf(_z_wbuf_t *buf)
{
    _z_wbuf_clear(buf);

#ifdef ZN_TRANSPORT_TCP_IP
    // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
    //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
    //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
    //       the boundary of the serialized messages. The length is encoded as little-endian.
    //       In any case, the length of a message must not exceed 65_535 bytes.
    for (size_t i = 0; i < _ZN_MSG_LEN_ENC_SIZE; ++i)
        _z_wbuf_put(buf, 0, i);

    _z_wbuf_set_wpos(buf, _ZN_MSG_LEN_ENC_SIZE);
#endif /* ZN_TRANSPORT_TCP_IP */
}

void _zn_finalize_wbuf(_z_wbuf_t *buf)
{
#ifdef ZN_TRANSPORT_TCP_IP
    // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
    //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
    //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
    //       the boundary of the serialized messages. The length is encoded as little-endian.
    //       In any case, the length of a message must not exceed 65_535 bytes.
    size_t len = _z_wbuf_len(buf) - _ZN_MSG_LEN_ENC_SIZE;
    for (size_t i = 0; i < _ZN_MSG_LEN_ENC_SIZE; ++i)
        _z_wbuf_put(buf, (uint8_t)((len >> 8 * i) & 0xFF), i);
#endif /* ZN_TRANSPORT_TCP_IP */
}

int _zn_send_s_msg(zn_session_t *zn, _zn_session_message_t *s_msg)
{
    _Z_DEBUG(">> send session message\n");

    // Acquire the lock
    _zn_mutex_lock(&zn->mutex_tx);
    // Prepare the buffer eventually reserving space for the message length
    _zn_prepare_wbuf(&zn->wbuf);
    // Encode the session message
    if (_zn_session_message_encode(&zn->wbuf, s_msg) != 0)
    {
        _Z_DEBUG("Dropping session message because it is too large");
        // Release the lock
        _zn_mutex_unlock(&zn->mutex_tx);
        return -1;
    }
    // Write the message legnth in the reserved space if needed
    _zn_finalize_wbuf(&zn->wbuf);
    // Send the wbuf on the socket
    int res = _zn_send_wbuf(zn->sock, &zn->wbuf);
    // Release the lock
    _zn_mutex_unlock(&zn->mutex_tx);

    // Mark the session that we have transmitted data
    zn->transmitted = 1;

    return res;
}

_zn_session_message_t _zn_frame_header(zn_reliability_t reliability, int is_fragment, int is_final, z_zint_t sn)
{
    // Create the frame session message that carries the zenoh message
    _zn_session_message_t s_msg = _zn_session_message_init(_ZN_MID_FRAME);
    s_msg.body.frame.sn = sn;

    if (reliability == zn_reliability_t_RELIABLE)
        _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_R);

    if (is_fragment)
    {
        _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_F);

        if (is_final)
            _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_E);

        // Do not add the payload
        s_msg.body.frame.payload.fragment.len = 0;
        s_msg.body.frame.payload.fragment.val = NULL;
    }
    else
    {
        // Do not allocate the vector containing the messages
        s_msg.body.frame.payload.messages._capacity = 0;
        s_msg.body.frame.payload.messages._len = 0;
        s_msg.body.frame.payload.messages._val = NULL;
    }

    return s_msg;
}

int _zn_serialize_zenoh_fragment(_z_wbuf_t *dst, _z_wbuf_t *src, zn_reliability_t reliability, size_t sn)
{
    // Assume first that this is not the final fragment
    int is_final = 0;
    do
    {
        // Mark the buffer for the writing operation
        size_t w_pos = _z_wbuf_get_wpos(dst);
        // Get the frame header
        _zn_session_message_t f_hdr = _zn_frame_header(reliability, 1, is_final, sn);
        // Encode the frame header
        int res = _zn_session_message_encode(dst, &f_hdr);
        if (res == 0)
        {
            size_t space_left = _z_wbuf_space_left(dst);
            size_t bytes_left = _z_wbuf_len(src);
            // Check if it is really the final fragment
            if (!is_final && (bytes_left <= space_left))
            {
                // Revert the buffer
                _z_wbuf_set_wpos(dst, w_pos);
                // It is really the finally fragment, reserialize the header
                is_final = 1;
                continue;
            }
            // Write the fragment
            size_t to_copy = bytes_left <= space_left ? bytes_left : space_left;
            return _z_wbuf_copy_into(dst, src, to_copy);
        }
        else
        {
            return 0;
        }
    } while (1);
}

int _zn_send_z_msg(zn_session_t *zn, _zn_zenoh_message_t *z_msg, zn_reliability_t reliability)
{
    _Z_DEBUG(">> send zenoh message\n");

    // Acquire the lock
    _zn_mutex_lock(&zn->mutex_tx);
    // Prepare the buffer eventually reserving space for the message length
    _zn_prepare_wbuf(&zn->wbuf);

    // Get the next sequence number
    size_t sn = _zn_get_sn(zn, reliability);
    // Create the frame header that carries the zenoh message
    _zn_session_message_t s_msg = _zn_frame_header(reliability, 0, 0, sn);

    // Encode the frame header
    int res = _zn_session_message_encode(&zn->wbuf, &s_msg);
    if (res != 0)
    {
        _Z_DEBUG("Dropping zenoh message because the session frame can not be encoded");
        // Release the lock
        _zn_mutex_unlock(&zn->mutex_tx);
        return -1;
    }

    // Encode the zenoh message
    res = _zn_zenoh_message_encode(&zn->wbuf, z_msg);
    if (res == 0)
    {
        // Write the message legnth in the reserved space if needed
        _zn_finalize_wbuf(&zn->wbuf);
        // Send the wbuf on the socket
        res = _zn_send_wbuf(zn->sock, &zn->wbuf);
        if (res == 0)
            // Mark the session that we have transmitted data
            zn->transmitted = 1;
    }
    else
    {
        // The message does not fit in the current batch, let's fragment it
        // Create an expandable wbuf for fragmentation
        _z_wbuf_t fbf = _z_wbuf_make(ZN_FRAG_BUF_TX_CHUNK, 1);

        // Encode the message on the expandable wbuf
        res = _zn_zenoh_message_encode(&fbf, z_msg);
        if (res != 0)
        {
            _Z_DEBUG("Dropping zenoh message because it can not be fragmented");
            goto EXIT_FRAG_PROC;
        }

        // Fragment and send the message
        int is_first = 1;
        while (_z_wbuf_len(&fbf) > 0)
        {
            // Get the fragment sequence number
            if (!is_first)
                sn = _zn_get_sn(zn, reliability);
            is_first = 0;

            // Clear the buffer for serialization
            _zn_prepare_wbuf(&zn->wbuf);

            // Serialize one fragment
            res = _zn_serialize_zenoh_fragment(&zn->wbuf, &fbf, reliability, sn);
            if (res != 0)
            {
                _Z_DEBUG("Dropping zenoh message because it can not be fragmented");
                goto EXIT_FRAG_PROC;
            }

            // Write the message length in the reserved space if needed
            _zn_finalize_wbuf(&zn->wbuf);

            // Send the wbuf on the socket
            res = _zn_send_wbuf(zn->sock, &zn->wbuf);
            if (res != 0)
            {
                _Z_DEBUG("Dropping zenoh message because it can not sent");
                goto EXIT_FRAG_PROC;
            }

            // Mark the session that we have transmitted data
            zn->transmitted = 1;
        }

    EXIT_FRAG_PROC:
        // Free the fragmentation buffer memory
        _z_wbuf_free(&fbf);
    }

    // Release the lock
    _zn_mutex_unlock(&zn->mutex_tx);

    return res;
}

void _zn_recv_s_msg_na(zn_session_t *zn, _zn_session_message_p_result_t *r)
{
    _Z_DEBUG(">> recv session msg\n");
    r->tag = _z_res_t_OK;

    // Acquire the lock
    _zn_mutex_lock(&zn->mutex_rx);
    // Prepare the buffer
    _z_rbuf_clear(&zn->rbuf);

#ifdef ZN_TRANSPORT_TCP_IP
    // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
    //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
    //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
    //       the boundary of the serialized messages. The length is encoded as little-endian.
    //       In any case, the length of a message must not exceed 65_535 bytes.

    // Read the message length
    if (_zn_recv_bytes(zn->sock, zn->rbuf.ios.buf, _ZN_MSG_LEN_ENC_SIZE) < 0)
    {
        // Release the lock
        _zn_mutex_lock(&zn->mutex_rx);
        _zn_session_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IO_GENERIC;
        return;
    }
    _z_rbuf_set_wpos(&zn->rbuf, _ZN_MSG_LEN_ENC_SIZE);

    uint16_t len = _z_rbuf_read(&zn->rbuf) | (_z_rbuf_read(&zn->rbuf) << 8);
    _Z_DEBUG_VA(">> \t msg len = %zu\n", len);
    size_t writable = _z_rbuf_capacity(&zn->rbuf) - _z_rbuf_len(&zn->rbuf);
    if (writable < len)
    {
        // Release the lock
        _zn_mutex_unlock(&zn->mutex_rx);
        _zn_session_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IOBUF_NO_SPACE;
        return;
    }

    // Read enough bytes to decode the message
    if (_zn_recv_bytes(zn->sock, zn->rbuf.ios.buf, len) < 0)
    {
        // Release the lock
        _zn_mutex_unlock(&zn->mutex_rx);
        _zn_session_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IO_GENERIC;
        return;
    }

    _z_rbuf_set_rpos(&zn->rbuf, 0);
    _z_rbuf_set_wpos(&zn->rbuf, len);
#else
    if (_zn_recv_buf(sock, buf) < 0)
    {
        // Release the lock
        _zn_mutex_unlock(&zn->mutex_rx);
        _zn_session_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IO_GENERIC;
        return;
    }
#endif /* ZN_TRANSPORT_TCP_IP */

    // Mark the session that we have received data
    zn->received = 1;

    _Z_DEBUG(">> \t session_message_decode\n");
    _zn_session_message_decode_na(&zn->rbuf, r);

    // Release the lock
    _zn_mutex_unlock(&zn->mutex_rx);
}

_zn_session_message_p_result_t _zn_recv_s_msg(zn_session_t *zn)
{
    _zn_session_message_p_result_t r;
    _zn_session_message_p_result_init(&r);
    _zn_recv_s_msg_na(zn, &r);
    return r;
}

/*------------------ Entity ------------------*/
z_zint_t _zn_get_entity_id(zn_session_t *zn)
{
    return zn->entity_id++;
}

/*------------------ Resource ------------------*/
z_zint_t _zn_get_resource_id(zn_session_t *zn, const zn_reskey_t *reskey)
{
    _zn_resource_t *res_decl = _zn_get_resource_by_key(zn, _ZN_IS_LOCAL, reskey);
    if (res_decl)
    {
        return res_decl->id;
    }
    else
    {
        z_zint_t id = zn->resource_id++;
        while (_zn_get_resource_by_id(zn, _ZN_IS_LOCAL, id) != 0)
        {
            id++;
        }
        zn->resource_id = id;
        return id;
    }
}

z_str_t _zn_get_resource_name_from_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    z_str_t rname = NULL;

    // Case 2) -> string only reskey, duplicate the rname
    if (reskey->rid == ZN_RESOURCE_ID_NONE)
    {
        rname = strdup(reskey->rname);
        return rname;
    }

    // Need to build the complete resource name
    _z_list_t *strs = NULL;
    size_t len = 0;

    if (reskey->rname)
    {
        // Case 3) -> numerical reskey with suffix, same as Case 1) but we first append the suffix
        len += strlen(reskey->rname);
        strs = _z_list_cons(strs, (void *)reskey->rname);
    }

    // Case 1) -> numerical only reskey
    z_zint_t id = reskey->rid;
    do
    {
        _zn_resource_t *res = _zn_get_resource_by_id(zn, is_local, id);
        if (res == NULL)
        {
            _z_list_free(&strs);
            return rname;
        }

        if (res->key.rname)
        {
            len += strlen(res->key.rname);
            strs = _z_list_cons(strs, (void *)res->key.rname);
        }

        id = res->key.rid;
    } while (id != ZN_RESOURCE_ID_NONE);

    // Concatenate all the partial resource names
    rname = (z_str_t)malloc(len + 1);
    // Start with a zero-length string to concatenate upon
    rname[0] = '\0';
    while (strs)
    {
        z_str_t s = (z_str_t)_z_list_head(strs);
        strcat(rname, s);
        strs = _z_list_pop(strs);
    }

    return rname;
}

_zn_resource_t *_zn_get_resource_by_id(zn_session_t *zn, int is_local, z_zint_t id)
{
    _z_list_t *decls = is_local ? zn->local_resources : zn->remote_resources;
    while (decls)
    {
        _zn_resource_t *decl = (_zn_resource_t *)_z_list_head(decls);

        if (decl->id == id)
            return decl;

        decls = _z_list_tail(decls);
    }

    return NULL;
}

_zn_resource_t *_zn_get_resource_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    _z_list_t *decls = is_local ? zn->local_resources : zn->remote_resources;
    while (decls)
    {
        _zn_resource_t *decl = (_zn_resource_t *)_z_list_head(decls);

        if (decl->key.rid == reskey->rid && strcmp(decl->key.rname, reskey->rname) == 0)
            return decl;

        decls = _z_list_tail(decls);
    }

    return NULL;
}

int _zn_register_resource(zn_session_t *zn, int is_local, _zn_resource_t *res)
{
    _Z_DEBUG_VA(">>> Allocating res decl for (%zu,%zu,%s)\n", res->id, res->key.rid, res->key.rname);
    _zn_resource_t *rd_rid = _zn_get_resource_by_id(zn, is_local, res->id);
    _zn_resource_t *rd_key = _zn_get_resource_by_key(zn, is_local, &res->key);

    if (!rd_rid && !rd_key)
    {
        // No resource declaration has been found, add the new one
        if (is_local)
            zn->local_resources = _z_list_cons(zn->local_resources, res);
        else
            zn->remote_resources = _z_list_cons(zn->remote_resources, res);

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
    _zn_resource_t *rel = (_zn_resource_t *)elem;
    _zn_resource_t *rar = (_zn_resource_t *)arg;
    if (rel->id == rar->id)
        return 1;
    else
        return 0;
}

void _zn_unregister_resource(zn_session_t *zn, int is_local, _zn_resource_t *res)
{
    if (is_local)
        zn->local_resources = _z_list_remove(zn->local_resources, _zn_resource_predicate, res);
    else
        zn->remote_resources = _z_list_remove(zn->remote_resources, _zn_resource_predicate, res);
}

/*------------------ Subscription ------------------*/
_z_list_t *_zn_get_subscriptions_from_remote_key(zn_session_t *zn, const zn_reskey_t *reskey)
{
    _z_list_t *xs = _z_list_empty;

    // Case 1) -> numerical only reskey
    if (reskey->rname == NULL)
    {
        _z_list_t *subs = (_z_list_t *)_z_i_map_get(zn->rem_res_loc_sub_map, reskey->rid);
        while (subs)
        {
            _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(subs);
            xs = _z_list_cons(xs, sub);
            subs = _z_list_tail(subs);
        }
        return xs;
    }
    // Case 2) -> string only reskey
    else if (reskey->rid == ZN_RESOURCE_ID_NONE)
    {
        // The complete resource name of the remote key
        z_str_t rname = reskey->rname;

        _z_list_t *subs = zn->local_subscriptions;
        while (subs)
        {
            _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(subs);

            // The complete resource name of the subscribed key
            z_str_t lname;
            if (sub->key.rid == ZN_RESOURCE_ID_NONE)
            {
                // Do not allocate
                lname = sub->key.rname;
            }
            else
            {
                // Allocate a computed string
                lname = _zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &sub->key);
                if (lname == NULL)
                {
                    _z_list_free(&xs);
                    xs = NULL;
                    return xs;
                }
            }

            if (zn_rname_intersect(lname, rname))
                xs = _z_list_cons(xs, sub);

            if (sub->key.rid != ZN_RESOURCE_ID_NONE)
                free(lname);

            subs = _z_list_tail(subs);
        }

        return xs;
    }
    // Case 3) -> numerical reskey with suffix
    else
    {
        _zn_resource_t *remote = _zn_get_resource_by_id(zn, _ZN_IS_REMOTE, reskey->rid);
        if (remote == NULL)
            return xs;

        // Compute the complete remote resource name starting from the key
        z_str_t rname = _zn_get_resource_name_from_key(zn, _ZN_IS_REMOTE, reskey);

        _z_list_t *subs = zn->local_subscriptions;
        _zn_subscriber_t *sub;
        while (subs)
        {
            sub = (_zn_subscriber_t *)_z_list_head(subs);

            // Get the complete resource name to be passed to the subscription callback
            z_str_t lname;
            if (sub->key.rid == ZN_RESOURCE_ID_NONE)
            {
                // Do not allocate
                lname = sub->key.rname;
            }
            else
            {
                // Allocate a computed string
                lname = _zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &sub->key);
                if (lname == NULL)
                    continue;
            }

            if (zn_rname_intersect(lname, rname))
                xs = _z_list_cons(xs, sub);

            if (sub->key.rid != ZN_RESOURCE_ID_NONE)
                free(lname);

            subs = _z_list_tail(subs);
        }

        free(rname);

        return xs;
    }
}

_zn_subscriber_t *_zn_get_subscription_by_id(zn_session_t *zn, int is_local, z_zint_t id)
{
    _z_list_t *subs = is_local ? zn->local_subscriptions : zn->remote_subscriptions;
    _zn_subscriber_t *sub;
    while (subs)
    {
        sub = (_zn_subscriber_t *)_z_list_head(subs);

        if (sub->id == id)
            return sub;

        subs = _z_list_tail(subs);
    }

    return NULL;
}

_zn_subscriber_t *_zn_get_subscription_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    _z_list_t *subs = is_local ? zn->local_subscriptions : zn->remote_subscriptions;

    _zn_subscriber_t *sub;
    while (subs)
    {
        sub = (_zn_subscriber_t *)_z_list_head(subs);

        if (sub->key.rid == reskey->rid && strcmp(sub->key.rname, reskey->rname) == 0)
            return sub;

        subs = _z_list_tail(subs);
    }

    return NULL;
}

int _zn_register_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *sub)
{
    _Z_DEBUG_VA(">>> Allocating sub decl for (%zu,%s)\n", reskey->rid, reskey->rname);

    _zn_subscriber_t *s = _zn_get_subscription_by_id(zn, is_local, sub->id);
    if (s)
    {
        // A subscription for this id already exists, return error
        return -1;
    }

    s = _zn_get_subscription_by_key(zn, is_local, &sub->key);
    if (s)
    {
        // A subscription for this key already exists, return error
        return -1;
    }

    // Register the new subscription
    if (is_local)
        zn->local_subscriptions = _z_list_cons(zn->local_subscriptions, sub);
    else
        zn->remote_subscriptions = _z_list_cons(zn->remote_subscriptions, sub);

    return 0;
}

int _zn_subscription_predicate(void *elem, void *arg)
{
    _zn_subscriber_t *s = (_zn_subscriber_t *)arg;
    _zn_subscriber_t *sub = (_zn_subscriber_t *)elem;
    if (sub->id == s->id)
    {
        free(sub->key.rname);
        return 1;
    }
    else
    {
        return 0;
    }
}

void _zn_unregister_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *s)
{
    if (is_local)
        zn->local_subscriptions = _z_list_remove(zn->local_subscriptions, _zn_subscription_predicate, s);
    else
        zn->remote_subscriptions = _z_list_remove(zn->remote_subscriptions, _zn_subscription_predicate, s);
}

void _zn_trigger_subscriptions(zn_session_t *zn, const zn_reskey_t reskey, const z_bytes_t payload)
{
    // Case 1) -> numeric only reskey
    if (reskey.rname == NULL)
    {
        // Get the declared resource
        _zn_resource_t *res = _zn_get_resource_by_id(zn, _ZN_IS_REMOTE, reskey.rid);
        if (res == NULL)
            return;

        // Get the complete resource name to be passed to the subscription callback
        z_str_t rname;
        if (res->key.rid == ZN_RESOURCE_ID_NONE)
        {
            // Do not allocate
            rname = res->key.rname;
        }
        else
        {
            // Allocate a computed string
            rname = _zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &res->key);
            if (rname == NULL)
                return;
        }

        // Build the sample
        zn_sample_t s;
        s.key.val = rname;
        s.key.len = strlen(s.key.val);
        s.value = payload;

        // Iterate over the matching subscriptions
        _z_list_t *subs = (_z_list_t *)_z_i_map_get(zn->rem_res_loc_sub_map, reskey.rid);
        while (subs)
        {
            _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(subs);
            sub->data_handler(&s, sub->arg);
            subs = _z_list_tail(subs);
        }

        if (res->key.rid != ZN_RESOURCE_ID_NONE)
            free(rname);

        return;
    }
    // Case 2) -> string only reskey
    else if (reskey.rid == ZN_RESOURCE_ID_NONE)
    {
        // Build the sample
        zn_sample_t s;
        s.key.val = reskey.rname;
        s.key.len = strlen(s.key.val);
        s.value = payload;

        _z_list_t *subs = zn->local_subscriptions;
        while (subs)
        {
            _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(subs);

            // Get the complete resource name to be passed to the subscription callback
            z_str_t rname;
            if (sub->key.rid == ZN_RESOURCE_ID_NONE)
            {
                // Do not allocate
                rname = sub->key.rname;
            }
            else
            {
                // Allocate a computed string
                rname = _zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &sub->key);
                if (rname == NULL)
                    continue;
            }

            if (zn_rname_intersect(rname, reskey.rname))
                sub->data_handler(&s, sub->arg);

            if (sub->key.rid != ZN_RESOURCE_ID_NONE)
                free(rname);

            subs = _z_list_tail(subs);
        }

        return;
    }
    // Case 3) -> numerical reskey with suffix
    else
    {
        _zn_resource_t *remote = _zn_get_resource_by_id(zn, _ZN_IS_REMOTE, reskey.rid);
        if (remote == NULL)
            return;

        // Compute the complete remote resource name starting from the key
        z_str_t rname = _zn_get_resource_name_from_key(zn, _ZN_IS_REMOTE, &reskey);

        // Build the sample
        zn_sample_t s;
        s.key.val = rname;
        s.key.len = strlen(s.key.val);
        s.value = payload;

        _z_list_t *subs = zn->local_subscriptions;
        _zn_subscriber_t *sub;
        while (subs)
        {
            sub = (_zn_subscriber_t *)_z_list_head(subs);

            // Get the complete resource name to be passed to the subscription callback
            z_str_t lname;
            if (sub->key.rid == ZN_RESOURCE_ID_NONE)
            {
                // Do not allocate
                lname = sub->key.rname;
            }
            else
            {
                // Allocate a computed string
                lname = _zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &sub->key);
                if (lname == NULL)
                    continue;
            }

            if (zn_rname_intersect(lname, rname))
                sub->data_handler(&s, sub->arg);

            if (sub->key.rid != ZN_RESOURCE_ID_NONE)
                free(lname);

            subs = _z_list_tail(subs);
        }

        free(rname);

        return;
    }
}

/*------------------ Query ------------------*/
z_zint_t _zn_get_query_id(zn_session_t *zn)
{
    return zn->query_id++;
}

/*------------------ Queryable ------------------*/
// void _zn_register_queryable(zn_session_t *zn, z_zint_t rid, z_zint_t id, zn_query_handler_t query_handler, void *arg)
// {
//     _zn_queryable_t *qle = (_zn_queryable_t *)malloc(sizeof(_zn_queryable_t));
//     qle->rid = rid;
//     qle->id = id;
//     _zn_resource_t *decl = _zn_get_res_decl_by_rid(zn, rid);
//     assert(decl != 0);
//     qle->rname = strdup(decl->key.rname);
//     qle->query_handler = query_handler;
//     qle->arg = arg;
//     zn->queryables = _z_list_cons(zn->queryables, qle);
// }

// int qle_predicate(void *elem, void *arg)
// {
//     zn_queryable_t *q = (zn_queryable_t *)arg;
//     _zn_queryable_t *queryable = (_zn_queryable_t *)elem;
//     if (queryable->id == q->id)
//     {
//         return 1;
//     }
//     else
//     {
//         return 0;
//     }
// }

// void _zn_unregister_queryable(zn_queryable_t *e)
// {
//     e->zn->queryables = _z_list_remove(e->zn->queryables, qle_predicate, e);
// }

// _z_list_t *_zn_get_queryables_by_rid(zn_session_t *zn, z_zint_t rid)
// {
//     _z_list_t *queryables = _z_list_empty;
//     if (zn->queryables == 0)
//     {
//         return queryables;
//     }
//     else
//     {
//         _zn_queryable_t *queryable = 0;
//         _z_list_t *queryables = zn->queryables;
//         _z_list_t *xs = _z_list_empty;
//         do
//         {
//             queryable = (_zn_queryable_t *)_z_list_head(queryables);
//             queryables = _z_list_tail(queryables);
//             if (queryable->rid == rid)
//             {
//                 xs = _z_list_cons(xs, queryable);
//             }
//         } while (queryables != 0);
//         return xs;
//     }
// }

// _z_list_t *_zn_get_queryables_by_rname(zn_session_t *zn, const char *rname)
// {
//     _z_list_t *queryables = _z_list_empty;
//     if (zn->queryables == 0)
//     {
//         return queryables;
//     }
//     else
//     {
//         _zn_queryable_t *queryable = 0;
//         _z_list_t *queryables = zn->queryables;
//         _z_list_t *xs = _z_list_empty;
//         do
//         {
//             queryable = (_zn_queryable_t *)_z_list_head(queryables);
//             queryables = _z_list_tail(queryables);
//             if (zn_rname_intersect(queryable->rname, (char *)rname))
//             {
//                 xs = _z_list_cons(xs, queryable);
//             }
//         } while (queryables != 0);
//         return xs;
//     }
// }

// int _zn_matching_remote_sub(zn_session_t *zn, z_zint_t rid)
// {
//     return _z_i_map_get(zn->remote_subscriptions, rid) != 0 ? 1 : 0;
// }

// void _zn_register_query(zn_session_t *zn, z_zint_t qid, zn_reply_handler_t reply_handler, void *arg)
// {
//     _zn_replywaiter_t *rw = (_zn_replywaiter_t *)malloc(sizeof(_zn_replywaiter_t));
//     rw->qid = qid;
//     rw->reply_handler = reply_handler;
//     rw->arg = arg;
//     zn->replywaiters = _z_list_cons(zn->replywaiters, rw);
// }

// _zn_replywaiter_t *_zn_get_query(zn_session_t *zn, z_zint_t qid)
// {
//     if (zn->replywaiters == 0)
//     {
//         return 0;
//     }
//     else
//     {
//         _zn_replywaiter_t *rw = (_zn_replywaiter_t *)_z_list_head(zn->replywaiters);
//         _z_list_t *rws = _z_list_tail(zn->replywaiters);
//         while (rws != 0 && rw->qid != qid)
//         {
//             rw = _z_list_head(rws);
//             rws = _z_list_tail(rws);
//         }
//         if (rw->qid == qid)
//             return rw;
//         else
//             return 0;
//     }
// }
