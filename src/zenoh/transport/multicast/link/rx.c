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

#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

int _z_vec_find_z_bytes_t(_z_vec_t *v, z_bytes_t *e)
{
    for (size_t i = 0; i < v->len; i++)
    {
        if (((z_bytes_t *)v->val[i])->len != e->len)
            return -1;

        if (memcmp(((z_bytes_t *)v->val[i])->val, e->val, e->len) == 0)
            return i;
    }

    return -1;
}

/*------------------ Reception helper ------------------*/
void _zn_multicast_recv_t_msg_na(_zn_transport_multicast_t *ztm, _zn_transport_message_result_t *r, z_bytes_t *addr)
{
    _Z_DEBUG(">> recv session msg\n");
    r->tag = _z_res_t_OK;

    // Acquire the lock
    z_mutex_lock(&ztm->mutex_rx);

    // Prepare the buffer
    _z_zbuf_clear(&ztm->zbuf);

    if (ztm->link->is_streamed == 1)
    {
        // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
        //       in bytes of the message, resulting in the maximum len gth of a message being 65_535 bytes.
        //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
        //       the boundary of the serialized messages. The length is encoded as little-endian.
        //       In any case, the length of a message must not exceed 65_535 bytes.

        // Read the message length
        if (_zn_link_recv_exact_zbuf(ztm->link, &ztm->zbuf, _ZN_MSG_LEN_ENC_SIZE, addr) != _ZN_MSG_LEN_ENC_SIZE)
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_IO_GENERIC;
            goto EXIT_SRCV_PROC;
        }

        uint16_t len = _z_zbuf_read(&ztm->zbuf) | (_z_zbuf_read(&ztm->zbuf) << 8);
        _Z_DEBUG_VA(">> \t msg len = %hu\n", len);
        size_t writable = _z_zbuf_capacity(&ztm->zbuf) - _z_zbuf_len(&ztm->zbuf);
        if (writable < len)
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_IOBUF_NO_SPACE;
            goto EXIT_SRCV_PROC;
        }

        // Read enough bytes to decode the message
        if (_zn_link_recv_exact_zbuf(ztm->link, &ztm->zbuf, len, addr) != len)
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_IO_GENERIC;
            goto EXIT_SRCV_PROC;
        }
    }
    else
    {
        if (_zn_link_recv_zbuf(ztm->link, &ztm->zbuf, addr) < 0)
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_IO_GENERIC;
            goto EXIT_SRCV_PROC;
        }
    }

    // Mark the session that we have received data
    ztm->received = 1;

    _Z_DEBUG(">> \t transport_message_decode\n");
    _zn_transport_message_decode_na(&ztm->zbuf, r);

EXIT_SRCV_PROC:
    // Release the lock
    z_mutex_unlock(&ztm->mutex_rx);
}

_zn_transport_message_result_t _zn_multicast_recv_t_msg(_zn_transport_multicast_t *ztm, z_bytes_t *addr)
{
    _zn_transport_message_result_t r;

    _zn_multicast_recv_t_msg_na(ztm, &r, addr);
    return r;
}

int _zn_multicast_handle_transport_message(_zn_transport_multicast_t *ztm, _zn_transport_message_t *t_msg, z_bytes_t *addr)
{
    switch (_ZN_MID(t_msg->header))
    {
    case _ZN_MID_SCOUT:
    {
        // Do nothing, multicast transports are not expected to handle SCOUT messages on established sessions
        return _z_res_t_OK;
    }

    case _ZN_MID_HELLO:
    {
        // Do nothing, multicast transports are not expected to handle HELLO messages on established sessions
        return _z_res_t_OK;
    }

    case _ZN_MID_INIT:
    {
        // Do nothing, multicas transports are not expected to handle INIT messages
        return _z_res_t_OK;
    }

    case _ZN_MID_OPEN:
    {
        // Do nothing, multicas transports are not expected to handle OPEN messages
        return _z_res_t_OK;
    }

    case _ZN_MID_JOIN:
    {
        if(_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_A))
        {
            if (t_msg->body.join.version != ZN_PROTO_VERSION)
                return _z_res_t_OK;
        }

        // Lookup for peer_id in internal struct
        int pos = _z_vec_find_z_bytes_t(&ztm->remote_addr_peers, addr);
        if (pos == -1) // New peer
        {
            _z_vec_append(&ztm->remote_addr_peers, addr);

            z_bytes_t *pid = (z_bytes_t *)malloc(sizeof(z_bytes_t));
            _z_bytes_copy(pid, &t_msg->body.join.pid);
            _z_vec_append(&ztm->remote_pid_peers, pid);

            z_zint_t *sn_resolution = (z_zint_t *)malloc(sizeof(z_zint_t));
            if(_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_S)) {
                *sn_resolution = t_msg->body.join.sn_resolution;
            }
            else
            {
                *sn_resolution = ZN_SN_RESOLUTION_DEFAULT;
            }
            z_zint_t *sn_resolution_half = (z_zint_t *)malloc(sizeof(z_zint_t));
            *sn_resolution_half = *sn_resolution / 2;
            _z_vec_append(&ztm->sn_resolution_peers, sn_resolution);
            _z_vec_append(&ztm->sn_resolution_half_peers, sn_resolution_half);

            if(t_msg->body.join.next_sns.is_qos == 0) // This shouls check on the flag as well
            {
                z_zint_t *sn_rx_best_effort = (z_zint_t *)malloc(sizeof(z_zint_t));
                z_zint_t *sn_rx_reliable = (z_zint_t *)malloc(sizeof(z_zint_t));
                *sn_rx_best_effort = _zn_sn_decrement(*sn_resolution, t_msg->body.join.next_sns.val.plain.best_effort);
                *sn_rx_reliable = _zn_sn_decrement(*sn_resolution, t_msg->body.join.next_sns.val.plain.reliable);

                _z_vec_append(&ztm->sn_rx_best_effort_peers, sn_rx_best_effort);
                _z_vec_append(&ztm->sn_rx_reliable_peers, sn_rx_reliable);
            }
            else
            {
                // FIXME: QoS is not assume to be supported for the moment
            }

            _z_wbuf_t *dbuf_best_effort = (_z_wbuf_t *)malloc(sizeof(_z_wbuf_t));
            *dbuf_best_effort = _z_wbuf_make(0, 1);
            _z_vec_append(&ztm->dbuf_best_effort_peers, dbuf_best_effort);

            _z_wbuf_t *dbuf_reliable = (_z_wbuf_t *)malloc(sizeof(_z_wbuf_t));
            *dbuf_reliable = _z_wbuf_make(0, 1);
            _z_vec_append(&ztm->dbuf_reliable_peers, dbuf_reliable);

            // TODO: Create peer-specific lease time
        }
        else // Existing peer
        {
            z_zint_t *sn_resolution = _z_vec_get(&ztm->sn_resolution_peers, pos);
            if(_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_S))
            {
                z_zint_t *sn_resolution_half = _z_vec_get(&ztm->sn_resolution_peers, pos);
                *sn_resolution = t_msg->body.join.sn_resolution;
                *sn_resolution_half = *sn_resolution / 2;
            }

            if(t_msg->body.join.next_sns.is_qos == 0)
            {
                z_zint_t *sn_rx_best_effort = _z_vec_get(&ztm->sn_rx_best_effort_peers, pos);
                z_zint_t *sn_rx_reliable = _z_vec_get(&ztm->sn_rx_reliable_peers, pos);
                *sn_rx_best_effort = _zn_sn_decrement(*sn_resolution, t_msg->body.join.next_sns.val.plain.best_effort);
                *sn_rx_reliable = _zn_sn_decrement(*sn_resolution, t_msg->body.join.next_sns.val.plain.reliable);
            }
            else
            {
                // FIXME: QoS is not assume to be supported for the moment
            }

            // TODO: Update lease time
        }

        return _z_res_t_OK;
    }

    case _ZN_MID_CLOSE:
    {
        _Z_DEBUG("Closing session as requested by the remote peer");
        return _z_res_t_OK;
    }

    case _ZN_MID_SYNC:
    {
        _Z_DEBUG("Handling of Sync messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_ACK_NACK:
    {
        _Z_DEBUG("Handling of AckNack messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_KEEP_ALIVE:
    {
        return _z_res_t_OK;
    }

    case _ZN_MID_PING_PONG:
    {
        _Z_DEBUG("Handling of PingPong messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_FRAME:
    {
        // Get peer transport params
        int pos = _z_vec_find_z_bytes_t(&ztm->remote_addr_peers, addr);
        if (pos == -1)
            return _z_res_t_OK;

        z_zint_t *sn_resolution = (z_zint_t *) _z_vec_get(&ztm->sn_resolution_peers, pos);
        z_zint_t *sn_resolution_half = (z_zint_t *) _z_vec_get(&ztm->sn_resolution_half_peers, pos);
        z_zint_t *sn_rx_reliable = (z_zint_t *) _z_vec_get(&ztm->sn_rx_reliable_peers, pos);
        z_zint_t *sn_rx_best_effort = (z_zint_t *) _z_vec_get(&ztm->sn_rx_best_effort_peers, pos);

        _z_wbuf_t *dbuf_reliable = (_z_wbuf_t *) _z_vec_get(&ztm->dbuf_best_effort_peers, pos);
        _z_wbuf_t *dbuf_best_effort = (_z_wbuf_t *) _z_vec_get(&ztm->dbuf_reliable_peers, pos);

        // Check if the SN is correct
        if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_R))
        {
            // @TODO: amend once reliability is in place. For the time being only
            //        monothonic SNs are ensured
            if (_zn_sn_precedes(*sn_resolution_half, *sn_rx_reliable, t_msg->body.frame.sn))
                *sn_rx_reliable = t_msg->body.frame.sn;
            else
            {
                _z_wbuf_clear(dbuf_reliable);
                _Z_DEBUG("Reliable message dropped because it is out of order");
                return _z_res_t_OK;
            }
        }
        else
        {
            if (_zn_sn_precedes(*sn_resolution_half, *sn_rx_best_effort, t_msg->body.frame.sn))
                *sn_rx_best_effort = t_msg->body.frame.sn;
            else
            {
                _z_wbuf_clear(dbuf_best_effort);
                _Z_DEBUG("Best effort message dropped because it is out of order");
                return _z_res_t_OK;
            }
        }

        if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_F))
        {
            int res = _z_res_t_OK;

            // Select the right defragmentation buffer
            _z_wbuf_t *dbuf = _ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_R) ? dbuf_reliable : dbuf_best_effort;
            // Add the fragment to the defragmentation buffer
            _z_wbuf_add_iosli_from(dbuf, t_msg->body.frame.payload.fragment.val, t_msg->body.frame.payload.fragment.len);

            // Check if this is the last fragment
            if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_E))
            {
                // Convert the defragmentation buffer into a decoding buffer
                _z_zbuf_t zbf = _z_wbuf_to_zbuf(dbuf);

                // Decode the zenoh message
                _zn_zenoh_message_result_t r_zm = _zn_zenoh_message_decode(&zbf);
                if (r_zm.tag == _z_res_t_OK)
                {
                    _zn_zenoh_message_t d_zm = r_zm.value.zenoh_message;
                    res = _zn_handle_zenoh_message(ztm->session, &d_zm);

                    // Free the decoded message
                    _zn_zenoh_message_free(&d_zm);
                }
                else
                {
                    res = _z_res_t_ERR;
                }

                // Free the decoding buffer
                _z_zbuf_clear(&zbf);
                // Reset the defragmentation buffer
                _z_wbuf_reset(dbuf);
            }

            return res;
        }
        else
        {
            // Handle all the zenoh message, one by one
            unsigned int len = _z_vec_len(&t_msg->body.frame.payload.messages);
            for (unsigned int i = 0; i < len; i++)
            {
                int res = _zn_handle_zenoh_message(ztm->session, (_zn_zenoh_message_t *)_z_vec_get(&t_msg->body.frame.payload.messages, i));
                if (res != _z_res_t_OK)
                    return res;
            }
            return _z_res_t_OK;
        }
    }

    default:
    {
        _Z_DEBUG("Unknown session message ID");
        return _z_res_t_ERR;
    }
    }
}