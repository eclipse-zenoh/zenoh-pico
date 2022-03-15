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

#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/utils/logging.h"

_zn_transport_peer_entry_t *_zn_find_peer_entry(_zn_transport_peer_entry_list_t *l, z_bytes_t *remote_addr)
{
    for (; l != NULL; l = l->tail)
    {
        if (((_zn_transport_peer_entry_t *)l->val)->remote_addr.len != remote_addr->len)
            continue;

        if (memcmp(((_zn_transport_peer_entry_t *)l->val)->remote_addr.val, remote_addr->val, remote_addr->len) == 0)
            return l->val;
    }

    return NULL;
}

/*------------------ Reception helper ------------------*/
void _zn_multicast_recv_t_msg_na(_zn_transport_multicast_t *ztm, _zn_transport_message_result_t *r, z_bytes_t *addr)
{
    _Z_DEBUG(">> recv session msg\n");
    r->tag = _z_res_t_OK;

    // Acquire the lock
    z_mutex_lock(&ztm->mutex_rx);

    // Prepare the buffer
    _z_zbuf_reset(&ztm->zbuf);

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
        _Z_DEBUG(">> \t msg len = %hu\n", len);
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
        if (_zn_link_recv_zbuf(ztm->link, &ztm->zbuf, addr) == SIZE_MAX)
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_IO_GENERIC;
            goto EXIT_SRCV_PROC;
        }
    }

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
    // Acquire and keep the lock
    z_mutex_lock(&ztm->mutex_peer);

    // Mark the session that we have received data from this peer
    _zn_transport_peer_entry_t *entry = _zn_find_peer_entry(ztm->peers, addr);
    switch (_ZN_MID(t_msg->header))
    {
    case _ZN_MID_SCOUT:
    {
        // Do nothing, multicast transports are not expected to handle SCOUT messages on established sessions
        break;
    }

    case _ZN_MID_HELLO:
    {
        // Do nothing, multicast transports are not expected to handle HELLO messages on established sessions
        break;
    }

    case _ZN_MID_INIT:
    {
        // Do nothing, multicas transports are not expected to handle INIT messages
        break;
    }

    case _ZN_MID_OPEN:
    {
        // Do nothing, multicas transports are not expected to handle OPEN messages
        break;
    }

    case _ZN_MID_JOIN:
    {
        _Z_INFO("Received _ZN_JOIN message\n");
        if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_A))
        {
            if (t_msg->body.join.version != ZN_PROTO_VERSION)
                break;
        }

        if (entry == NULL) // New peer
        {
            entry = (_zn_transport_peer_entry_t *)malloc(sizeof(_zn_transport_peer_entry_t));
            entry->remote_addr = _z_bytes_duplicate(addr);
            entry->remote_pid = _z_bytes_duplicate(&t_msg->body.join.pid);
            if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_S))
                entry->sn_resolution = t_msg->body.join.sn_resolution;
            else
                entry->sn_resolution = ZN_SN_RESOLUTION_DEFAULT;
            entry->sn_resolution_half = entry->sn_resolution / 2;

            _zn_conduit_sn_list_copy(&entry->sn_rx_sns, &t_msg->body.join.next_sns);
            _zn_conduit_sn_list_decrement(entry->sn_resolution, &entry->sn_rx_sns);

            entry->dbuf_best_effort = _z_wbuf_make(0, 1);
            entry->dbuf_reliable = _z_wbuf_make(0, 1);

            // Update lease time (set as ms during)
            entry->lease = t_msg->body.join.lease;
            entry->next_lease = entry->lease;
            entry->received = 1;

            ztm->peers = _zn_transport_peer_entry_list_push(ztm->peers, entry);
        }
        else // Existing peer
        {
            entry->received = 1;

            // Check if the sn resolution remains the same
            if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_S) && (entry->sn_resolution != t_msg->body.join.sn_resolution))
            {
                _zn_transport_peer_entry_list_drop_filter(ztm->peers, _zn_transport_peer_entry_eq, entry);
                break;
            }

            // Update SNs
            _zn_conduit_sn_list_copy(&entry->sn_rx_sns, &t_msg->body.join.next_sns);
            _zn_conduit_sn_list_decrement(entry->sn_resolution, &entry->sn_rx_sns);

            // Update lease time (set as ms during)
            entry->lease = t_msg->body.join.lease;
        }
        break;
    }

    case _ZN_MID_CLOSE:
    {
        _Z_INFO("Closing session as requested by the remote peer\n");

        if (entry == NULL)
            break;

        if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_I))
        {
            // Check if the Peer ID matches the remote address in the knonw peer list
            if (entry->remote_pid.len != t_msg->body.close.pid.len || memcmp(entry->remote_pid.val, t_msg->body.close.pid.val, entry->remote_pid.len) != 0)
                break;
        }
        ztm->peers = _zn_transport_peer_entry_list_drop_filter(ztm->peers, _zn_transport_peer_entry_eq, entry);

        break;
    }

    case _ZN_MID_SYNC:
    {
        _Z_INFO("Handling of Sync messages not implemented\n");
        break;
    }

    case _ZN_MID_ACK_NACK:
    {
        _Z_INFO("Handling of AckNack messages not implemented\n");
        break;
    }

    case _ZN_MID_KEEP_ALIVE:
    {
        _Z_INFO("Received _ZN_KEEP_ALIVE message\n");
        if (entry == NULL)
            break;
        entry->received = 1;

        break;
    }

    case _ZN_MID_PING_PONG:
    {
        _Z_DEBUG("Handling of PingPong messages not implemented");
        break;
    }

    case _ZN_MID_FRAME:
    {
        _Z_INFO("Received _ZN_FRAME message\n");
        if (entry == NULL)
            break;
        entry->received = 1;

        // Check if the SN is correct
        if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_R))
        {
            // @TODO: amend once reliability is in place. For the time being only
            //        monothonic SNs are ensured
            if (_zn_sn_precedes(entry->sn_resolution_half, entry->sn_rx_sns.val.plain.reliable, t_msg->body.frame.sn))
                entry->sn_rx_sns.val.plain.reliable = t_msg->body.frame.sn;
            else
            {
                _z_wbuf_clear(&entry->dbuf_reliable);
                _Z_INFO("Reliable message dropped because it is out of order");
                break;
            }
        }
        else
        {
            if (_zn_sn_precedes(entry->sn_resolution_half, entry->sn_rx_sns.val.plain.best_effort, t_msg->body.frame.sn))
                entry->sn_rx_sns.val.plain.best_effort = t_msg->body.frame.sn;
            else
            {
                _z_wbuf_clear(&entry->dbuf_best_effort);
                _Z_INFO("Best effort message dropped because it is out of order");
                break;
            }
        }

        if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_F))
        {
            int res = _z_res_t_OK;

            // Select the right defragmentation buffer
            _z_wbuf_t *dbuf = _ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_R) ? &entry->dbuf_reliable : &entry->dbuf_best_effort;
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
                    _zn_z_msg_clear(&d_zm);
                }

                // Free the decoding buffer
                _z_zbuf_reset(&zbf);
                // Reset the defragmentation buffer
                _z_wbuf_reset(dbuf);
            }

            if (res != _z_res_t_OK)
                break;
        }
        else
        {
            // Handle all the zenoh message, one by one
            unsigned int len = _z_vec_len(&t_msg->body.frame.payload.messages);
            for (unsigned int i = 0; i < len; i++)
            {
                int res = _zn_handle_zenoh_message(ztm->session, (_zn_zenoh_message_t *)_z_vec_get(&t_msg->body.frame.payload.messages, i));
                if (res != _z_res_t_OK)
                    break;
            }
            break;
        }
    }

    default:
    {
        _Z_ERROR("Unknown session message ID\n");
        break;
    }
    }

    z_mutex_unlock(&ztm->mutex_peer);
    return _z_res_t_OK;
}
