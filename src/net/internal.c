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

#include "zenoh-pico/net/private/internal.h"
#include "zenoh-pico/net/private/msg.h"
#include "zenoh-pico/net/private/msgcodec.h"
#include "zenoh-pico/net/private/system.h"
#include "zenoh-pico/net/rname.h"
#include "zenoh-pico/private/logging.h"
#include "zenoh-pico/private/system.h"
#include "zenoh-pico/utils.h"

/*------------------ Clone helpers ------------------*/
zn_reskey_t _zn_reskey_clone(const zn_reskey_t *reskey)
{
    zn_reskey_t rk;
    rk.rid = reskey->rid,
    rk.rname = reskey->rname ? strdup(reskey->rname) : NULL;
    return rk;
}

z_timestamp_t _z_timestamp_clone(const z_timestamp_t *tstamp)
{
    z_timestamp_t ts;
    _z_bytes_copy(&ts.id, &tstamp->id);
    ts.time = tstamp->time;
    return ts;
}

void _z_timestamp_reset(z_timestamp_t *tstamp)
{
    _z_bytes_reset(&tstamp->id);
    tstamp->time = 0;
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

_zn_reply_context_t *_zn_reply_context_init(void)
{
    _zn_reply_context_t *rc = (_zn_reply_context_t *)malloc(sizeof(_zn_reply_context_t));
    memset(rc, 0, sizeof(_zn_reply_context_t));
    rc->header = _ZN_MID_REPLY_CONTEXT;
    return rc;
}

_zn_attachment_t *_zn_attachment_init(void)
{
    _zn_attachment_t *att = (_zn_attachment_t *)malloc(sizeof(_zn_attachment_t));
    memset(att, 0, sizeof(_zn_attachment_t));
    att->header = _ZN_MID_ATTACHMENT;
    return att;
}

/*------------------ SN helper ------------------*/
/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
z_zint_t __unsafe_zn_get_sn(zn_session_t *zn, zn_reliability_t reliability)
{
    z_zint_t sn;
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
/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_tx
 */
void __unsafe_zn_prepare_wbuf(_z_wbuf_t *buf)
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

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_tx
 */
void __unsafe_zn_finalize_wbuf(_z_wbuf_t *buf)
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
    _z_mutex_lock(&zn->mutex_tx);

    // Prepare the buffer eventually reserving space for the message length
    __unsafe_zn_prepare_wbuf(&zn->wbuf);
    // Encode the session message
    int res = _zn_session_message_encode(&zn->wbuf, s_msg);
    if (res == 0)
    {
        // Write the message legnth in the reserved space if needed
        __unsafe_zn_finalize_wbuf(&zn->wbuf);
        // Send the wbuf on the socket
        res = _zn_send_wbuf(zn->sock, &zn->wbuf);
        // Mark the session that we have transmitted data
        zn->transmitted = 1;
    }
    else
    {
        _Z_DEBUG("Dropping session message because it is too large");
    }

    // Release the lock
    _z_mutex_unlock(&zn->mutex_tx);

    return res;
}

_zn_session_message_t __zn_frame_header(zn_reliability_t reliability, int is_fragment, int is_final, z_zint_t sn)
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

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_tx
 */
int __unsafe_zn_serialize_zenoh_fragment(_z_wbuf_t *dst, _z_wbuf_t *src, zn_reliability_t reliability, size_t sn)
{
    // Assume first that this is not the final fragment
    int is_final = 0;
    do
    {
        // Mark the buffer for the writing operation
        size_t w_pos = _z_wbuf_get_wpos(dst);
        // Get the frame header
        _zn_session_message_t f_hdr = __zn_frame_header(reliability, 1, is_final, sn);
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

int _zn_send_z_msg(zn_session_t *zn, _zn_zenoh_message_t *z_msg, zn_reliability_t reliability, zn_congestion_control_t cong_ctrl)
{
    _Z_DEBUG(">> send zenoh message\n");

    // Acquire the lock and drop the message if needed
    if (cong_ctrl == zn_congestion_control_t_BLOCK)
    {
        _z_mutex_lock(&zn->mutex_tx);
    }
    else
    {
        int locked = _z_mutex_trylock(&zn->mutex_tx);
        if (locked != 0)
        {
            _Z_DEBUG("Dropping zenoh message because of congestion control\n");
            // We failed to acquire the lock, drop the message
            return 0;
        }
    }

    // Prepare the buffer eventually reserving space for the message length
    __unsafe_zn_prepare_wbuf(&zn->wbuf);

    // Get the next sequence number
    z_zint_t sn = __unsafe_zn_get_sn(zn, reliability);
    // Create the frame header that carries the zenoh message
    _zn_session_message_t s_msg = __zn_frame_header(reliability, 0, 0, sn);

    // Encode the frame header
    int res = _zn_session_message_encode(&zn->wbuf, &s_msg);
    if (res != 0)
    {
        _Z_DEBUG("Dropping zenoh message because the session frame can not be encoded\n");
        goto EXIT_ZSND_PROC;
    }

    // Encode the zenoh message
    res = _zn_zenoh_message_encode(&zn->wbuf, z_msg);
    if (res == 0)
    {
        // Write the message legnth in the reserved space if needed
        __unsafe_zn_finalize_wbuf(&zn->wbuf);

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
                sn = __unsafe_zn_get_sn(zn, reliability);
            is_first = 0;

            // Clear the buffer for serialization
            __unsafe_zn_prepare_wbuf(&zn->wbuf);

            // Serialize one fragment
            res = __unsafe_zn_serialize_zenoh_fragment(&zn->wbuf, &fbf, reliability, sn);
            if (res != 0)
            {
                _Z_DEBUG("Dropping zenoh message because it can not be fragmented\n");
                goto EXIT_FRAG_PROC;
            }

            // Write the message length in the reserved space if needed
            __unsafe_zn_finalize_wbuf(&zn->wbuf);

            // Send the wbuf on the socket
            res = _zn_send_wbuf(zn->sock, &zn->wbuf);
            if (res != 0)
            {
                _Z_DEBUG("Dropping zenoh message because it can not sent\n");
                goto EXIT_FRAG_PROC;
            }

            // Mark the session that we have transmitted data
            zn->transmitted = 1;
        }

    EXIT_FRAG_PROC:
        // Free the fragmentation buffer memory
        _z_wbuf_free(&fbf);
    }

EXIT_ZSND_PROC:
    // Release the lock
    _z_mutex_unlock(&zn->mutex_tx);

    return res;
}

void _zn_recv_s_msg_na(zn_session_t *zn, _zn_session_message_p_result_t *r)
{
    _Z_DEBUG(">> recv session msg\n");
    r->tag = _z_res_t_OK;

    // Acquire the lock
    _z_mutex_lock(&zn->mutex_rx);

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
        _zn_session_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IO_GENERIC;
        goto EXIT_SRCV_PROC;
    }
    _z_rbuf_set_wpos(&zn->rbuf, _ZN_MSG_LEN_ENC_SIZE);

    uint16_t len = _z_rbuf_read(&zn->rbuf) | (_z_rbuf_read(&zn->rbuf) << 8);
    _Z_DEBUG_VA(">> \t msg len = %hu\n", len);
    size_t writable = _z_rbuf_capacity(&zn->rbuf) - _z_rbuf_len(&zn->rbuf);
    if (writable < len)
    {
        _zn_session_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IOBUF_NO_SPACE;
        goto EXIT_SRCV_PROC;
    }

    // Read enough bytes to decode the message
    if (_zn_recv_bytes(zn->sock, zn->rbuf.ios.buf, len) < 0)
    {
        _zn_session_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IO_GENERIC;
        goto EXIT_SRCV_PROC;
    }

    _z_rbuf_set_rpos(&zn->rbuf, 0);
    _z_rbuf_set_wpos(&zn->rbuf, len);
#else
    if (_zn_recv_buf(sock, buf) < 0)
    {
        _zn_session_message_p_result_free(r);
        r->tag = _z_res_t_ERR;
        r->value.error = _zn_err_t_IO_GENERIC;
        goto EXIT_SRCV_PROC;
    }
#endif /* ZN_TRANSPORT_TCP_IP */

    // Mark the session that we have received data
    zn->received = 1;

    _Z_DEBUG(">> \t session_message_decode\n");
    _zn_session_message_decode_na(&zn->rbuf, r);

EXIT_SRCV_PROC:
    // Release the lock
    _z_mutex_unlock(&zn->mutex_rx);
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
/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_resource_t *__unsafe_zn_get_resource_by_id(zn_session_t *zn, int is_local, z_zint_t id)
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

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_resource_t *__unsafe_zn_get_resource_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
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

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
z_str_t __unsafe_zn_get_resource_name_from_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
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
        _zn_resource_t *res = __unsafe_zn_get_resource_by_id(zn, is_local, id);
        if (res == NULL)
        {
            _z_list_free(strs);
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

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_resource_t *__unsafe_zn_get_resource_matching_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    _z_list_t *decls = is_local ? zn->local_resources : zn->remote_resources;

    z_str_t rname;
    if (reskey->rid == ZN_RESOURCE_ID_NONE)
        rname = reskey->rname;
    else
        rname = __unsafe_zn_get_resource_name_from_key(zn, is_local, reskey);

    while (decls)
    {
        _zn_resource_t *decl = (_zn_resource_t *)_z_list_head(decls);

        z_str_t lname;
        if (decl->key.rid == ZN_RESOURCE_ID_NONE)
            lname = decl->key.rname;
        else
            lname = __unsafe_zn_get_resource_name_from_key(zn, is_local, &decl->key);

        // Verify if it intersects
        int res = zn_rname_intersect(lname, rname);

        // Free the resource key if allocated
        if (decl->key.rid != ZN_RESOURCE_ID_NONE)
            free(lname);

        // Exit if it inersects
        if (res)
        {
            if (reskey->rid != ZN_RESOURCE_ID_NONE)
                free(rname);
            return decl;
        }

        decls = _z_list_tail(decls);
    }

    if (reskey->rid != ZN_RESOURCE_ID_NONE)
        free(rname);

    return NULL;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_z_list_t *__unsafe_zn_get_subscriptions_from_remote_key(zn_session_t *zn, const zn_reskey_t *reskey)
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
                lname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &sub->key);
                if (lname == NULL)
                {
                    _z_list_free(xs);
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
    }
    // Case 3) -> numerical reskey with suffix
    else
    {
        _zn_resource_t *remote = __unsafe_zn_get_resource_by_id(zn, _ZN_IS_REMOTE, reskey->rid);
        if (remote == NULL)
            return xs;

        // Compute the complete remote resource name starting from the key
        z_str_t rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_REMOTE, reskey);

        _z_list_t *subs = zn->local_subscriptions;
        while (subs)
        {
            _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(subs);

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
                lname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &sub->key);
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
    }

    return xs;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_add_rem_res_to_loc_sub_map(zn_session_t *zn, z_zint_t id, zn_reskey_t *reskey)
{
    // Check if there is a matching local subscription
    _z_list_t *subs = __unsafe_zn_get_subscriptions_from_remote_key(zn, reskey);
    if (subs)
    {
        // Update the list
        _z_list_t *sl = _z_i_map_get(zn->rem_res_loc_sub_map, id);
        if (sl)
        {
            // Free any ancient list
            _z_list_free(sl);
        }
        // Update the list of active subscriptions
        _z_i_map_set(zn->rem_res_loc_sub_map, id, subs);
    }
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_z_list_t *__unsafe_zn_get_queryables_from_remote_key(zn_session_t *zn, const zn_reskey_t *reskey)
{
    _z_list_t *xs = _z_list_empty;
    // Case 1) -> numerical only reskey
    if (reskey->rname == NULL)
    {
        _z_list_t *qles = (_z_list_t *)_z_i_map_get(zn->rem_res_loc_qle_map, reskey->rid);
        while (qles)
        {
            _zn_queryable_t *qle = (_zn_queryable_t *)_z_list_head(qles);
            xs = _z_list_cons(xs, qle);
            qles = _z_list_tail(qles);
        }
    }
    // Case 2) -> string only reskey
    else if (reskey->rid == ZN_RESOURCE_ID_NONE)
    {
        // The complete resource name of the remote key
        z_str_t rname = reskey->rname;

        _z_list_t *qles = zn->local_queryables;
        while (qles)
        {
            _zn_queryable_t *qle = (_zn_queryable_t *)_z_list_head(qles);

            // The complete resource name of the subscribed key
            z_str_t lname;
            if (qle->key.rid == ZN_RESOURCE_ID_NONE)
            {
                // Do not allocate
                lname = qle->key.rname;
            }
            else
            {
                // Allocate a computed string
                lname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &qle->key);
                if (lname == NULL)
                {
                    _z_list_free(xs);
                    xs = NULL;
                    return xs;
                }
            }

            if (zn_rname_intersect(lname, rname))
                xs = _z_list_cons(xs, qle);

            if (qle->key.rid != ZN_RESOURCE_ID_NONE)
                free(lname);

            qles = _z_list_tail(qles);
        }
    }
    // Case 3) -> numerical reskey with suffix
    else
    {
        _zn_resource_t *remote = __unsafe_zn_get_resource_by_id(zn, _ZN_IS_REMOTE, reskey->rid);
        if (remote == NULL)
            return xs;

        // Compute the complete remote resource name starting from the key
        z_str_t rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_REMOTE, reskey);

        _z_list_t *qles = zn->local_queryables;
        while (qles)
        {
            _zn_queryable_t *qle = (_zn_queryable_t *)_z_list_head(qles);

            // Get the complete resource name to be passed to the subscription callback
            z_str_t lname;
            if (qle->key.rid == ZN_RESOURCE_ID_NONE)
            {
                // Do not allocate
                lname = qle->key.rname;
            }
            else
            {
                // Allocate a computed string
                lname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &qle->key);
                if (lname == NULL)
                    continue;
            }

            if (zn_rname_intersect(lname, rname))
                xs = _z_list_cons(xs, qle);

            if (qle->key.rid != ZN_RESOURCE_ID_NONE)
                free(lname);

            qles = _z_list_tail(qles);
        }

        free(rname);
    }

    return xs;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_add_rem_res_to_loc_qle_map(zn_session_t *zn, z_zint_t id, zn_reskey_t *reskey)
{
    // Check if there is a matching local subscription
    _z_list_t *qles = __unsafe_zn_get_queryables_from_remote_key(zn, reskey);
    if (qles)
    {
        // Update the list
        _z_list_t *ql = _z_i_map_get(zn->rem_res_loc_qle_map, id);
        if (ql)
        {
            // Free any ancient list
            _z_list_free(ql);
        }
        // Update the list of active subscriptions
        _z_i_map_set(zn->rem_res_loc_qle_map, id, qles);
    }
}

z_zint_t _zn_get_resource_id(zn_session_t *zn)
{
    return zn->resource_id++;
}

_zn_resource_t *_zn_get_resource_by_id(zn_session_t *zn, int is_local, z_zint_t rid)
{
    // Lock the resources data struct
    _z_mutex_lock(&zn->mutex_inner);

    _zn_resource_t *res = __unsafe_zn_get_resource_by_id(zn, is_local, rid);

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
    return res;
}

_zn_resource_t *_zn_get_resource_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    _z_mutex_lock(&zn->mutex_inner);
    _zn_resource_t *res = __unsafe_zn_get_resource_by_key(zn, is_local, reskey);
    _z_mutex_unlock(&zn->mutex_inner);
    return res;
}

z_str_t _zn_get_resource_name_from_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    _z_mutex_lock(&zn->mutex_inner);
    z_str_t res = __unsafe_zn_get_resource_name_from_key(zn, is_local, reskey);
    _z_mutex_unlock(&zn->mutex_inner);
    return res;
}

_zn_resource_t *_zn_get_resource_matching_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    _z_mutex_lock(&zn->mutex_inner);
    _zn_resource_t *res = __unsafe_zn_get_resource_matching_key(zn, is_local, reskey);
    _z_mutex_unlock(&zn->mutex_inner);
    return res;
}

int _zn_register_resource(zn_session_t *zn, int is_local, _zn_resource_t *res)
{
    _Z_DEBUG_VA(">>> Allocating res decl for (%zu,%lu,%s)\n", res->id, res->key.rid, res->key.rname);

    // Lock the resources data struct
    _z_mutex_lock(&zn->mutex_inner);

    _zn_resource_t *rd_rid = __unsafe_zn_get_resource_by_id(zn, is_local, res->id);

    int r;
    if (rd_rid)
    {
        // Inconsistent declarations have been found, return an error
        r = -1;
    }
    else
    {
        // No resource declaration has been found, add the new one
        if (is_local)
        {
            zn->local_resources = _z_list_cons(zn->local_resources, res);
        }
        else
        {
            __unsafe_zn_add_rem_res_to_loc_sub_map(zn, res->id, &res->key);
            __unsafe_zn_add_rem_res_to_loc_qle_map(zn, res->id, &res->key);
            zn->remote_resources = _z_list_cons(zn->remote_resources, res);
        }

        r = 0;
    }

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);

    return r;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_free_resource(_zn_resource_t *res)
{
    _zn_reskey_free(&res->key);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
int __unsafe_zn_resource_predicate(void *other, void *this)
{
    _zn_resource_t *o = (_zn_resource_t *)other;
    _zn_resource_t *t = (_zn_resource_t *)this;
    if (t->id == o->id)
    {
        __unsafe_zn_free_resource(t);
        return 1;
    }
    else
    {
        return 0;
    }
}

void _zn_unregister_resource(zn_session_t *zn, int is_local, _zn_resource_t *res)
{
    // Lock the resources data struct
    _z_mutex_lock(&zn->mutex_inner);

    if (is_local)
        zn->local_resources = _z_list_remove(zn->local_resources, __unsafe_zn_resource_predicate, res);
    else
        zn->remote_resources = _z_list_remove(zn->remote_resources, __unsafe_zn_resource_predicate, res);
    free(res);

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}

void _zn_flush_resources(zn_session_t *zn)
{
    // Lock the resources data struct
    _z_mutex_lock(&zn->mutex_inner);

    while (zn->local_resources)
    {
        _zn_resource_t *res = (_zn_resource_t *)_z_list_head(zn->local_resources);
        __unsafe_zn_free_resource(res);
        free(res);
        zn->local_resources = _z_list_pop(zn->local_resources);
    }

    while (zn->remote_resources)
    {
        _zn_resource_t *res = (_zn_resource_t *)_z_list_head(zn->remote_resources);
        __unsafe_zn_free_resource(res);
        free(res);
        zn->remote_resources = _z_list_pop(zn->remote_resources);
    }

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}

/*------------------ Subscription ------------------*/
/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_subscriber_t *__unsafe_zn_get_subscription_by_id(zn_session_t *zn, int is_local, z_zint_t id)
{
    _z_list_t *subs = is_local ? zn->local_subscriptions : zn->remote_subscriptions;
    while (subs)
    {
        _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(subs);

        if (sub->id == id)
            return sub;

        subs = _z_list_tail(subs);
    }

    return NULL;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_subscriber_t *__unsafe_zn_get_subscription_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    _z_list_t *subs = is_local ? zn->local_subscriptions : zn->remote_subscriptions;
    while (subs)
    {
        _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(subs);

        if (sub->key.rid == reskey->rid && strcmp(sub->key.rname, reskey->rname) == 0)
            return sub;

        subs = _z_list_tail(subs);
    }

    return NULL;
}

_zn_subscriber_t *_zn_get_subscription_by_id(zn_session_t *zn, int is_local, z_zint_t id)
{
    // Acquire the lock on the subscriptions data struct
    _z_mutex_lock(&zn->mutex_inner);
    _zn_subscriber_t *sub = __unsafe_zn_get_subscription_by_id(zn, is_local, id);
    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
    return sub;
}

_zn_subscriber_t *_zn_get_subscription_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    // Acquire the lock on the subscriptions data struct
    _z_mutex_lock(&zn->mutex_inner);
    _zn_subscriber_t *sub = __unsafe_zn_get_subscription_by_key(zn, is_local, reskey);
    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
    return sub;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_add_loc_sub_to_rem_res_map(zn_session_t *zn, _zn_subscriber_t *sub)
{
    // Need to check if there is a remote resource declaration matching the new subscription
    zn_reskey_t loc_key;
    loc_key.rid = ZN_RESOURCE_ID_NONE;
    if (sub->key.rid == ZN_RESOURCE_ID_NONE)
        loc_key.rname = sub->key.rname;
    else
        loc_key.rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &sub->key);

    _zn_resource_t *rem_res = __unsafe_zn_get_resource_matching_key(zn, _ZN_IS_REMOTE, &loc_key);
    if (rem_res)
    {
        // Update the list of active subscriptions
        _z_list_t *subs = _z_i_map_get(zn->rem_res_loc_sub_map, rem_res->id);
        subs = _z_list_cons(subs, sub);
        _z_i_map_set(zn->rem_res_loc_sub_map, rem_res->id, subs);
    }

    if (sub->key.rid != ZN_RESOURCE_ID_NONE)
        free(loc_key.rname);
}

_z_list_t *_zn_get_subscriptions_from_remote_key(zn_session_t *zn, const zn_reskey_t *reskey)
{
    // Acquire the lock on the subscriptions data struct
    _z_mutex_lock(&zn->mutex_inner);
    _z_list_t *xs = __unsafe_zn_get_subscriptions_from_remote_key(zn, reskey);
    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
    return xs;
}

int _zn_register_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *sub)
{
    _Z_DEBUG_VA(">>> Allocating sub decl for (%lu,%s)\n", sub->key.rid, sub->key.rname);

    // Acquire the lock on the subscriptions data struct
    _z_mutex_lock(&zn->mutex_inner);

    int res;
    _zn_subscriber_t *s = __unsafe_zn_get_subscription_by_key(zn, is_local, &sub->key);
    if (s)
    {
        // A subscription for this key already exists, return error
        res = -1;
    }
    else
    {
        // Register the new subscription
        if (is_local)
        {
            __unsafe_zn_add_loc_sub_to_rem_res_map(zn, sub);
            zn->local_subscriptions = _z_list_cons(zn->local_subscriptions, sub);
        }
        else
        {
            zn->remote_subscriptions = _z_list_cons(zn->remote_subscriptions, sub);
        }
        res = 0;
    }

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);

    return res;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_free_subscription(_zn_subscriber_t *sub)
{
    _zn_reskey_free(&sub->key);
    if (sub->info.period)
        free(sub->info.period);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
int __unsafe_zn_subscription_predicate(void *other, void *this)
{
    _zn_subscriber_t *o = (_zn_subscriber_t *)other;
    _zn_subscriber_t *t = (_zn_subscriber_t *)this;
    if (t->id == o->id)
    {
        __unsafe_zn_free_subscription(t);
        return 1;
    }
    else
    {
        return 0;
    }
}

void _zn_unregister_subscription(zn_session_t *zn, int is_local, _zn_subscriber_t *s)
{
    // Acquire the lock on the subscription list
    _z_mutex_lock(&zn->mutex_inner);

    if (is_local)
        zn->local_subscriptions = _z_list_remove(zn->local_subscriptions, __unsafe_zn_subscription_predicate, s);
    else
        zn->remote_subscriptions = _z_list_remove(zn->remote_subscriptions, __unsafe_zn_subscription_predicate, s);
    free(s);

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}

void _zn_flush_subscriptions(zn_session_t *zn)
{
    // Lock the resources data struct
    _z_mutex_lock(&zn->mutex_inner);

    while (zn->local_subscriptions)
    {
        _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(zn->local_subscriptions);
        __unsafe_zn_free_subscription(sub);
        free(sub);
        zn->local_subscriptions = _z_list_pop(zn->local_subscriptions);
    }

    while (zn->remote_subscriptions)
    {
        _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(zn->remote_subscriptions);
        __unsafe_zn_free_subscription(sub);
        free(sub);
        zn->remote_subscriptions = _z_list_pop(zn->remote_subscriptions);
    }
    _z_i_map_free(zn->rem_res_loc_sub_map);

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}

void _zn_trigger_subscriptions(zn_session_t *zn, const zn_reskey_t reskey, const z_bytes_t payload)
{
    // Acquire the lock on the subscription list
    _z_mutex_lock(&zn->mutex_inner);

    // Case 1) -> numeric only reskey
    if (reskey.rname == NULL)
    {
        // Get the declared resource
        _zn_resource_t *res = __unsafe_zn_get_resource_by_id(zn, _ZN_IS_REMOTE, reskey.rid);
        if (res == NULL)
            goto EXIT_SUB_TRIG;

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
            rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &res->key);
            if (rname == NULL)
                goto EXIT_SUB_TRIG;
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
            sub->callback(&s, sub->arg);
            subs = _z_list_tail(subs);
        }

        if (res->key.rid != ZN_RESOURCE_ID_NONE)
            free(rname);
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
                rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &sub->key);
                if (rname == NULL)
                    continue;
            }

            if (zn_rname_intersect(rname, reskey.rname))
                sub->callback(&s, sub->arg);

            if (sub->key.rid != ZN_RESOURCE_ID_NONE)
                free(rname);

            subs = _z_list_tail(subs);
        }
    }
    // Case 3) -> numerical reskey with suffix
    else
    {
        // Compute the complete remote resource name starting from the key
        z_str_t rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_REMOTE, &reskey);
        if (rname == NULL)
            goto EXIT_SUB_TRIG;

        // Build the sample
        zn_sample_t s;
        s.key.val = rname;
        s.key.len = strlen(s.key.val);
        s.value = payload;

        _z_list_t *subs = zn->local_subscriptions;
        while (subs)
        {
            _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(subs);

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
                lname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &sub->key);
                if (lname == NULL)
                    continue;
            }

            if (zn_rname_intersect(lname, rname))
                sub->callback(&s, sub->arg);

            if (sub->key.rid != ZN_RESOURCE_ID_NONE)
                free(lname);

            subs = _z_list_tail(subs);
        }

        free(rname);
    }

EXIT_SUB_TRIG:
    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}

/*------------------ Pull ------------------*/
z_zint_t _zn_get_pull_id(zn_session_t *zn)
{
    return zn->pull_id++;
}

/*------------------ Query ------------------*/
z_zint_t _zn_get_query_id(zn_session_t *zn)
{
    return zn->query_id++;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_pending_query_t *__unsafe_zn_get_pending_query_by_id(zn_session_t *zn, z_zint_t id)
{
    _z_list_t *queries = zn->pending_queries;
    while (queries)
    {
        _zn_pending_query_t *query = (_zn_pending_query_t *)_z_list_head(queries);

        if (query->id == id)
            return query;

        queries = _z_list_tail(queries);
    }

    return NULL;
}

int _zn_register_pending_query(zn_session_t *zn, _zn_pending_query_t *pen_qry)
{
    _Z_DEBUG_VA(">>> Allocating query for (%lu,%s,%s)\n", pen_qry->key.rid, pen_qry->key.rname, pen_qry->predicate);
    // Acquire the lock on the queries
    _z_mutex_lock(&zn->mutex_inner);
    int res;
    _zn_pending_query_t *q = __unsafe_zn_get_pending_query_by_id(zn, pen_qry->id);
    if (q)
    {
        // A query for this id already exists, return error
        res = -1;
    }
    else
    {
        // Register the query
        zn->pending_queries = _z_list_cons(zn->pending_queries, pen_qry);
        res = 0;
    }

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);

    return res;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_free_pending_reply(_zn_pending_reply_t *pr)
{
    // Free the sample
    if (pr->reply.data.data.key.val)
        free((z_str_t)pr->reply.data.data.key.val);
    if (pr->reply.data.data.value.val)
        _z_bytes_free(&pr->reply.data.data.value);

    // Free the source info
    if (pr->reply.data.replier_id.val)
        _z_bytes_free(&pr->reply.data.replier_id);

    // Free the timestamp
    if (pr->tstamp.id.val)
        _z_bytes_free(&pr->tstamp.id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_free_pending_query(_zn_pending_query_t *pen_qry)
{
    _zn_reskey_free(&pen_qry->key);
    if (pen_qry->predicate)
        free((z_str_t)pen_qry->predicate);

    while (pen_qry->pending_replies)
    {
        _zn_pending_reply_t *pen_rep = (_zn_pending_reply_t *)_z_list_head(pen_qry->pending_replies);
        __unsafe_zn_free_pending_reply(pen_rep);
        pen_qry->pending_replies = _z_list_pop(pen_qry->pending_replies);
    }
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
int __unsafe_zn_pending_query_predicate(void *other, void *this)
{
    _zn_pending_query_t *o = (_zn_pending_query_t *)other;
    _zn_pending_query_t *t = (_zn_pending_query_t *)this;
    if (t->id == o->id)
    {
        __unsafe_zn_free_pending_query(t);
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_unregister_pending_query(zn_session_t *zn, _zn_pending_query_t *pen_qry)
{
    zn->pending_queries = _z_list_remove(zn->pending_queries, __unsafe_zn_pending_query_predicate, pen_qry);
    free(pen_qry);
}

void _zn_unregister_pending_query(zn_session_t *zn, _zn_pending_query_t *pen_qry)
{
    _z_mutex_lock(&zn->mutex_inner);
    __unsafe_zn_unregister_pending_query(zn, pen_qry);
    _z_mutex_unlock(&zn->mutex_inner);
}

void _zn_flush_pending_queries(zn_session_t *zn)
{
    // Lock the resources data struct
    _z_mutex_lock(&zn->mutex_inner);

    while (zn->pending_queries)
    {
        _zn_pending_query_t *pqy = (_zn_pending_query_t *)_z_list_head(zn->pending_queries);
        while (pqy->pending_replies)
        {
            _zn_pending_reply_t *pre = (_zn_pending_reply_t *)_z_list_head(pqy->pending_replies);
            __unsafe_zn_free_pending_reply(pre);
            free(pre);
            pqy->pending_replies = _z_list_pop(pqy->pending_replies);
        }
        __unsafe_zn_free_pending_query(pqy);
        free(pqy);
        zn->pending_queries = _z_list_pop(zn->pending_queries);
    }

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}

void _zn_trigger_query_reply_partial(zn_session_t *zn,
                                     const _zn_reply_context_t *reply_context,
                                     const zn_reskey_t reskey,
                                     const z_bytes_t payload,
                                     const _zn_data_info_t data_info)
{
    // Acquire the lock on the queries
    _z_mutex_lock(&zn->mutex_inner);

    if (_ZN_HAS_FLAG(reply_context->header, _ZN_FLAG_Z_F))
    {
        _Z_DEBUG(">>> Partial reply received with invalid final flag\n");
        goto EXIT_QRY_TRIG_PAR;
    }

    _zn_pending_query_t *pen_qry = __unsafe_zn_get_pending_query_by_id(zn, reply_context->qid);
    if (pen_qry == NULL)
    {
        _Z_DEBUG_VA(">>> Partial reply received for unkwon query id (%zu)\n", reply_context->qid);
        goto EXIT_QRY_TRIG_PAR;
    }

    if (pen_qry->target.kind != ZN_QUERYABLE_ALL_KINDS && (pen_qry->target.kind & reply_context->source_kind) == 0)
    {
        _Z_DEBUG_VA(">>> Partial reply received from an unknown target (%zu)\n", reply_context->source_kind);
        goto EXIT_QRY_TRIG_PAR;
    }

    // Take the right timestamp, or default to none
    z_timestamp_t ts;
    if _ZN_HAS_FLAG (data_info.flags, _ZN_DATA_INFO_TSTAMP)
        ts = data_info.tstamp;
    else
        _z_timestamp_reset(&ts);

    // Build the reply
    zn_reply_t reply;
    reply.tag = zn_reply_t_Tag_DATA;
    reply.data.data.value = payload;
    if (reskey.rid == ZN_RESOURCE_ID_NONE)
        reply.data.data.key.val = reskey.rname;
    else
        reply.data.data.key.val = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_REMOTE, &reskey);
    reply.data.data.key.len = strlen(reply.data.data.key.val);
    reply.data.replier_id = reply_context->replier_id;
    reply.data.source_kind = reply_context->source_kind;

    // Verify if this is a newer reply, free the old one in case it is
    _zn_pending_reply_t *latest = NULL;
    switch (pen_qry->consolidation.reception)
    {
    case zn_consolidation_mode_t_FULL:
    case zn_consolidation_mode_t_LAZY:
    {
        // Check if this is a newer reply
        _z_list_t *pen_rps = pen_qry->pending_replies;
        while (pen_rps)
        {
            _zn_pending_reply_t *pen_rep = (_zn_pending_reply_t *)_z_list_head(pen_rps);

            // Check if this is the same resource key
            if (strcmp(reply.data.data.key.val, pen_rep->reply.data.data.key.val) == 0)
            {
                if (ts.time <= pen_rep->tstamp.time)
                {
                    _Z_DEBUG(">>> Reply received with old timestamp\n");
                    if (reskey.rid != ZN_RESOURCE_ID_NONE)
                        free((z_str_t)reply.data.data.key.val);
                    goto EXIT_QRY_TRIG_PAR;
                }
                else
                {
                    // We are going to have a more recent reply, free the old one
                    __unsafe_zn_free_pending_reply(pen_rep);
                    // We are going to reuse the allocated memory in the list
                    latest = pen_rep;
                    break;
                }
            }
            else
            {
                pen_rps = _z_list_tail(pen_rps);
            }
        }
        break;
    }
    case zn_consolidation_mode_t_NONE:
    {
        // Do nothing. Replies are not stored with no consolidation
        break;
    }
    }

    // Store the reply and trigger the callback if needed
    switch (pen_qry->consolidation.reception)
    {
    // Store the reply but do not trigger the callback
    case zn_consolidation_mode_t_FULL:
    {
        // Allocate a pending reply if needed
        _zn_pending_reply_t *pen_rep;
        if (latest == NULL)
            pen_rep = (_zn_pending_reply_t *)malloc(sizeof(_zn_pending_reply_t));
        else
            pen_rep = latest;

        // Copy the reply tag
        pen_rep->reply.tag = reply.tag;

        // Make a copy of the sample if needed
        _z_bytes_copy((z_bytes_t *)&pen_rep->reply.data.data.value, (z_bytes_t *)&reply.data.data.value);
        if (reskey.rid == ZN_RESOURCE_ID_NONE)
            pen_rep->reply.data.data.key.val = strdup(reply.data.data.key.val);
        else
            pen_rep->reply.data.data.key.val = reply.data.data.key.val;
        pen_rep->reply.data.data.key.len = reply.data.data.key.len;

        // Make a copy of the source info
        _z_bytes_copy((z_bytes_t *)&pen_rep->reply.data.replier_id, (z_bytes_t *)&reply.data.replier_id);
        pen_rep->reply.data.source_kind = reply.data.source_kind;

        // Make a copy of the data info timestamp if present
        pen_rep->tstamp = _z_timestamp_clone(&ts);

        // Add it to the list of pending replies if new
        if (latest == NULL)
            pen_qry->pending_replies = _z_list_cons(pen_qry->pending_replies, pen_rep);

        break;
    }
    // Trigger the callback, store only the timestamp of the reply
    case zn_consolidation_mode_t_LAZY:
    {
        // Allocate a pending reply if needed
        _zn_pending_reply_t *pen_rep;
        if (latest == NULL)
            pen_rep = (_zn_pending_reply_t *)malloc(sizeof(_zn_pending_reply_t));
        else
            pen_rep = latest;

        // Copy the reply tag
        pen_rep->reply.tag = reply.tag;

        // Do not copy the payload, we are triggering the handler straight away
        // Copy the resource key
        pen_rep->reply.data.data.value = payload;
        if (reskey.rid == ZN_RESOURCE_ID_NONE)
            pen_rep->reply.data.data.key.val = strdup(reply.data.data.key.val);
        else
            pen_rep->reply.data.data.key.val = reply.data.data.key.val;
        pen_rep->reply.data.data.key.len = reply.data.data.key.len;

        // Do not copy the source info, we are triggering the handler straight away
        pen_rep->reply.data.replier_id = reply.data.replier_id;
        pen_rep->reply.data.source_kind = reply.data.source_kind;

        // Do not sotre the replier ID, we are triggering the handler straight away
        // Make a copy of the timestamp
        _z_bytes_reset(&pen_rep->tstamp.id);
        pen_rep->tstamp.time = ts.time;

        // Add it to the list of pending replies
        if (latest == NULL)
            pen_qry->pending_replies = _z_list_cons(pen_qry->pending_replies, pen_rep);

        // Trigger the handler
        pen_qry->callback(pen_rep->reply, pen_qry->arg);

        // Set to null the data and replier id
        _z_bytes_reset(&pen_rep->reply.data.data.value);
        _z_bytes_reset(&pen_rep->reply.data.replier_id);

        break;
    }
    // Trigger only the callback, do not store the reply
    case zn_consolidation_mode_t_NONE:
    {
        // Trigger the handler
        pen_qry->callback(reply, pen_qry->arg);

        // Free the resource name if allocated
        if (reskey.rid != ZN_RESOURCE_ID_NONE)
            free((char *)reply.data.data.key.val);

        break;
    }
    default:
        break;
    }

EXIT_QRY_TRIG_PAR:
    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}

void _zn_trigger_query_reply_final(zn_session_t *zn, const _zn_reply_context_t *reply_context)
{
    // Acquire the lock on the queries
    _z_mutex_lock(&zn->mutex_inner);

    if (!_ZN_HAS_FLAG(reply_context->header, _ZN_FLAG_Z_F))
    {
        _Z_DEBUG(">>> Final reply received with invalid final flag\n");
        goto EXIT_QRY_TRIG_FIN;
    }

    _zn_pending_query_t *pen_qry = __unsafe_zn_get_pending_query_by_id(zn, reply_context->qid);
    if (pen_qry == NULL)
    {
        _Z_DEBUG_VA(">>> Final reply received for unkwon query id (%zu)\n", reply_context->qid);
        goto EXIT_QRY_TRIG_FIN;
    }

    if (pen_qry->target.kind != ZN_QUERYABLE_ALL_KINDS && (pen_qry->target.kind & reply_context->source_kind) == 0)
    {
        _Z_DEBUG_VA(">>> Final reply received from an unknown target (%zu)\n", reply_context->source_kind);
        goto EXIT_QRY_TRIG_FIN;
    }

    // The reply is the final one, apply consolidation if needed
    while (pen_qry->pending_replies)
    {
        _zn_pending_reply_t *pen_rep = (_zn_pending_reply_t *)_z_list_head(pen_qry->pending_replies);
        if (pen_qry->consolidation.reception == zn_consolidation_mode_t_FULL)
        {
            // Trigger the query handler
            pen_qry->callback(pen_rep->reply, pen_qry->arg);
        }
        // Free the element
        __unsafe_zn_free_pending_reply(pen_rep);
        free(pen_rep);
        pen_qry->pending_replies = _z_list_pop(pen_qry->pending_replies);
    }

    // Build the final reply
    zn_reply_t fin_rep;
    memset(&fin_rep, 0, sizeof(zn_reply_t));
    fin_rep.tag = zn_reply_t_Tag_FINAL;
    // Trigger the final query handler
    pen_qry->callback(fin_rep, pen_qry->arg);

    __unsafe_zn_unregister_pending_query(zn, pen_qry);

EXIT_QRY_TRIG_FIN:
    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}

/*------------------ Queryable ------------------*/
/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_queryable_t *__unsafe_zn_get_queryable_by_id(zn_session_t *zn, z_zint_t id)
{
    _z_list_t *queryables = zn->local_queryables;
    while (queryables)
    {
        _zn_queryable_t *queryable = (_zn_queryable_t *)_z_list_head(queryables);

        if (queryable->id == id)
            return queryable;

        queryables = _z_list_tail(queryables);
    }

    return NULL;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_add_loc_qle_to_rem_res_map(zn_session_t *zn, _zn_queryable_t *qle)
{
    // Need to check if there is a remote resource declaration matching the new subscription
    zn_reskey_t loc_key;
    loc_key.rid = ZN_RESOURCE_ID_NONE;
    if (qle->key.rid == ZN_RESOURCE_ID_NONE)
        loc_key.rname = qle->key.rname;
    else
        loc_key.rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &qle->key);

    _zn_resource_t *rem_res = __unsafe_zn_get_resource_matching_key(zn, _ZN_IS_REMOTE, &loc_key);
    if (rem_res)
    {
        // Update the list of active subscriptions
        _z_list_t *qles = _z_i_map_get(zn->rem_res_loc_qle_map, rem_res->id);
        qles = _z_list_cons(qles, qle);
        _z_i_map_set(zn->rem_res_loc_qle_map, rem_res->id, qles);
    }

    if (qle->key.rid != ZN_RESOURCE_ID_NONE)
        free(loc_key.rname);
}

_zn_queryable_t *_zn_get_queryable_by_id(zn_session_t *zn, z_zint_t id)
{
    _z_mutex_lock(&zn->mutex_inner);
    _zn_queryable_t *qle = __unsafe_zn_get_queryable_by_id(zn, id);
    _z_mutex_unlock(&zn->mutex_inner);
    return qle;
}

int _zn_register_queryable(zn_session_t *zn, _zn_queryable_t *qle)
{
    _Z_DEBUG_VA(">>> Allocating queryable for (%lu,%s,%u)\n", qle->key.rid, qle->key.rname, qle->kind);

    // Acquire the lock on the queryables
    _z_mutex_lock(&zn->mutex_inner);

    int res;
    _zn_queryable_t *q = __unsafe_zn_get_queryable_by_id(zn, qle->id);
    if (q)
    {
        // A queryable for this id already exists, return error
        res = -1;
    }
    else
    {
        // Register the queryable
        __unsafe_zn_add_loc_qle_to_rem_res_map(zn, qle);
        zn->local_queryables = _z_list_cons(zn->local_queryables, qle);
        res = 0;
    }

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);

    return res;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_free_queryable(_zn_queryable_t *qle)
{
    _zn_reskey_free(&qle->key);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
int __unsafe_zn_queryable_predicate(void *other, void *this)
{
    _zn_queryable_t *o = (_zn_queryable_t *)other;
    _zn_queryable_t *t = (_zn_queryable_t *)this;
    if (t->id == o->id)
    {
        __unsafe_zn_free_queryable(t);
        return 1;
    }
    else
    {
        return 0;
    }
}

void _zn_unregister_queryable(zn_session_t *zn, _zn_queryable_t *qle)
{
    // Acquire the lock on the queryables
    _z_mutex_lock(&zn->mutex_inner);

    zn->local_queryables = _z_list_remove(zn->local_queryables, __unsafe_zn_queryable_predicate, qle);
    free(qle);

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}

void _zn_flush_queryables(zn_session_t *zn)
{
    // Lock the resources data struct
    _z_mutex_lock(&zn->mutex_inner);

    while (zn->local_queryables)
    {
        _zn_queryable_t *qle = (_zn_queryable_t *)_z_list_head(zn->local_queryables);
        __unsafe_zn_free_queryable(qle);
        free(qle);
        zn->local_queryables = _z_list_pop(zn->local_queryables);
    }
    _z_i_map_free(zn->rem_res_loc_qle_map);

    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}

void _zn_trigger_queryables(zn_session_t *zn, const _zn_query_t *query)
{
    // Acquire the lock on the queryables
    _z_mutex_lock(&zn->mutex_inner);

    // Case 1) -> numeric only reskey
    if (query->key.rname == NULL)
    {
        // Get the declared resource
        _zn_resource_t *res = __unsafe_zn_get_resource_by_id(zn, _ZN_IS_REMOTE, query->key.rid);
        if (res == NULL)
            goto EXIT_QLE_TRIG;

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
            rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &res->key);
            if (rname == NULL)
                goto EXIT_QLE_TRIG;
        }

        // Build the query
        zn_query_t q;
        q.zn = zn;
        q.qid = query->qid;
        q.rname = rname;
        q.predicate = query->predicate;

        // Iterate over the matching queryables
        _z_list_t *qles = (_z_list_t *)_z_i_map_get(zn->rem_res_loc_qle_map, query->key.rid);
        while (qles)
        {
            _zn_queryable_t *qle = (_zn_queryable_t *)_z_list_head(qles);
            unsigned int target = (query->target.kind & ZN_QUERYABLE_ALL_KINDS) | (query->target.kind & qle->kind);
            if (target != 0)
            {
                q.kind = qle->kind;
                qle->callback(&q, qle->arg);
            }
            qles = _z_list_tail(qles);
        }

        if (res->key.rid != ZN_RESOURCE_ID_NONE)
            free(rname);
    }
    // Case 2) -> string only reskey
    else if (query->key.rid == ZN_RESOURCE_ID_NONE)
    {
        // Build the query
        zn_query_t q;
        q.zn = zn;
        q.qid = query->qid;
        q.rname = query->key.rname;
        q.predicate = query->predicate;

        // Iterate over the matching queryables
        _z_list_t *qles = zn->local_queryables;
        while (qles)
        {
            _zn_queryable_t *qle = (_zn_queryable_t *)_z_list_head(qles);

            unsigned int target = (query->target.kind & ZN_QUERYABLE_ALL_KINDS) | (query->target.kind & qle->kind);
            if (target != 0)
            {
                // Get the complete resource name to be passed to the subscription callback
                z_str_t rname;
                if (qle->key.rid == ZN_RESOURCE_ID_NONE)
                {
                    // Do not allocate
                    rname = qle->key.rname;
                }
                else
                {
                    // Allocate a computed string
                    rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &qle->key);
                    if (rname == NULL)
                        continue;
                }

                if (zn_rname_intersect(rname, query->key.rname))
                {
                    q.kind = qle->kind;
                    qle->callback(&q, qle->arg);
                }

                if (qle->key.rid != ZN_RESOURCE_ID_NONE)
                    free(rname);
            }

            qles = _z_list_tail(qles);
        }
    }
    // Case 3) -> numerical reskey with suffix
    else
    {
        // Compute the complete remote resource name starting from the key
        z_str_t rname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_REMOTE, &query->key);
        if (rname == NULL)
            goto EXIT_QLE_TRIG;

        // Build the query
        zn_query_t q;
        q.zn = zn;
        q.qid = query->qid;
        q.rname = query->key.rname;
        q.predicate = query->predicate;

        _z_list_t *qles = zn->local_queryables;
        while (qles)
        {
            _zn_queryable_t *qle = (_zn_queryable_t *)_z_list_head(qles);

            unsigned int target = (query->target.kind & ZN_QUERYABLE_ALL_KINDS) | (query->target.kind & qle->kind);
            if (target != 0)
            {
                // Get the complete resource name to be passed to the subscription callback
                z_str_t lname;
                if (qle->key.rid == ZN_RESOURCE_ID_NONE)
                {
                    // Do not allocate
                    lname = qle->key.rname;
                }
                else
                {
                    // Allocate a computed string
                    lname = __unsafe_zn_get_resource_name_from_key(zn, _ZN_IS_LOCAL, &qle->key);
                    if (lname == NULL)
                        continue;
                }

                if (zn_rname_intersect(lname, rname))
                {
                    q.kind = qle->kind;
                    qle->callback(&q, qle->arg);
                }

                if (qle->key.rid != ZN_RESOURCE_ID_NONE)
                    free(lname);
            }

            qles = _z_list_tail(qles);
        }

        free(rname);
    }

    // Send the final reply
    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_UNIT);
    z_msg.reply_context = _zn_reply_context_init();
    _ZN_SET_FLAG(z_msg.reply_context->header, _ZN_FLAG_Z_F);
    z_msg.reply_context->qid = query->qid;
    z_msg.reply_context->source_kind = 0;

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        _Z_DEBUG("Trying to reconnect...\n");
        zn->on_disconnect(zn);
        _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
    }

    _zn_zenoh_message_free(&z_msg);

EXIT_QLE_TRIG:
    // Release the lock
    _z_mutex_unlock(&zn->mutex_inner);
}
