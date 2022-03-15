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
#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Reception helper ------------------*/
void _zn_unicast_recv_t_msg_na(_zn_transport_unicast_t *ztu, _zn_transport_message_result_t *r)
{
    _Z_DEBUG(">> recv session msg\n");
    r->tag = _z_res_t_OK;

    // Acquire the lock
    z_mutex_lock(&ztu->mutex_rx);

    // Prepare the buffer
    _z_zbuf_reset(&ztu->zbuf);

    if (ztu->link->is_streamed == 1)
    {
        // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
        //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
        //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
        //       the boundary of the serialized messages. The length is encoded as little-endian.
        //       In any case, the length of a message must not exceed 65_535 bytes.

        // Read the message length
        if (_zn_link_recv_exact_zbuf(ztu->link, &ztu->zbuf, _ZN_MSG_LEN_ENC_SIZE, NULL) != _ZN_MSG_LEN_ENC_SIZE)
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_IO_GENERIC;
            goto EXIT_SRCV_PROC;
        }

        uint16_t len = _z_zbuf_read(&ztu->zbuf) | (_z_zbuf_read(&ztu->zbuf) << 8);
        _Z_DEBUG(">> \t msg len = %hu\n", len);
        size_t writable = _z_zbuf_capacity(&ztu->zbuf) - _z_zbuf_len(&ztu->zbuf);
        if (writable < len)
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_IOBUF_NO_SPACE;
            goto EXIT_SRCV_PROC;
        }

        // Read enough bytes to decode the message
        if (_zn_link_recv_exact_zbuf(ztu->link, &ztu->zbuf, len, NULL) != len)
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_IO_GENERIC;
            goto EXIT_SRCV_PROC;
        }
    }
    else
    {
        if (_zn_link_recv_zbuf(ztu->link, &ztu->zbuf, NULL) == SIZE_MAX)
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_IO_GENERIC;
            goto EXIT_SRCV_PROC;
        }
    }

    // Mark the session that we have received data
    ztu->received = 1;

    _Z_DEBUG(">> \t transport_message_decode\n");
    _zn_transport_message_decode_na(&ztu->zbuf, r);

EXIT_SRCV_PROC:
    // Release the lock
    z_mutex_unlock(&ztu->mutex_rx);
}

_zn_transport_message_result_t _zn_unicast_recv_t_msg(_zn_transport_unicast_t *ztu)
{
    _zn_transport_message_result_t r;

    _zn_unicast_recv_t_msg_na(ztu, &r);
    return r;
}

int _zn_unicast_handle_transport_message(_zn_transport_unicast_t *ztu, _zn_transport_message_t *t_msg)
{
    switch (_ZN_MID(t_msg->header))
    {
    case _ZN_MID_SCOUT:
    {
        _Z_INFO("Handling of Scout messages not implemented\n");
        return _z_res_t_OK;
    }

    case _ZN_MID_HELLO:
    {
        // Do nothing, zenoh-pico clients are not expected to handle hello messages
        return _z_res_t_OK;
    }

    case _ZN_MID_INIT:
    {
        // Do nothing, zenoh clients are not expected to handle accept messages on established sessions
        return _z_res_t_OK;
    }

    case _ZN_MID_OPEN:
    {
        // Do nothing, zenoh clients are not expected to handle accept messages on established sessions
        return _z_res_t_OK;
    }

    case _ZN_MID_CLOSE:
    {
        _Z_INFO("Closing session as requested by the remote peer\n");
        return _z_res_t_OK;
    }

    case _ZN_MID_SYNC:
    {
        _Z_INFO("Handling of Sync messages not implemented\n");
        return _z_res_t_OK;
    }

    case _ZN_MID_ACK_NACK:
    {
        _Z_INFO("Handling of AckNack messages not implemented\n");
        return _z_res_t_OK;
    }

    case _ZN_MID_KEEP_ALIVE:
    {
        _Z_INFO("Received ZN_KEEP_ALIVE message\n");
        return _z_res_t_OK;
    }

    case _ZN_MID_PING_PONG:
    {
        _Z_INFO("Handling of PingPong messages not implemented\n");
        return _z_res_t_OK;
    }

    case _ZN_MID_FRAME:
    {
        _Z_INFO("Received ZN_FRAME message\n");
        // Check if the SN is correct
        if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_R))
        {
            // @TODO: amend once reliability is in place. For the time being only
            //        monothonic SNs are ensured
            if (_zn_sn_precedes(ztu->sn_resolution_half, ztu->sn_rx_reliable, t_msg->body.frame.sn))
            {
                ztu->sn_rx_reliable = t_msg->body.frame.sn;
            }
            else
            {
                _z_wbuf_clear(&ztu->dbuf_reliable);
                _Z_INFO("Reliable message dropped because it is out of order\n");
                return _z_res_t_OK;
            }
        }
        else
        {
            if (_zn_sn_precedes(ztu->sn_resolution_half, ztu->sn_rx_best_effort, t_msg->body.frame.sn))
            {
                ztu->sn_rx_best_effort = t_msg->body.frame.sn;
            }
            else
            {
                _z_wbuf_clear(&ztu->dbuf_best_effort);
                _Z_INFO("Best effort message dropped because it is out of order\n");
                return _z_res_t_OK;
            }
        }

        if (_ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_F))
        {
            int res = _z_res_t_OK;

            // Select the right defragmentation buffer
            _z_wbuf_t *dbuf = _ZN_HAS_FLAG(t_msg->header, _ZN_FLAG_T_R) ? &ztu->dbuf_reliable : &ztu->dbuf_best_effort;
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
                    res = _zn_handle_zenoh_message(ztu->session, &d_zm);

                    // Free the decoded message
                    _zn_z_msg_clear(&d_zm);
                }
                else
                {
                    res = _z_res_t_ERR;
                }

                // Free the decoding buffer
                _z_zbuf_reset(&zbf);
                // Reset the defragmentation buffer
                _z_wbuf_clear(dbuf);
            }

            return res;
        }
        else
        {
            // Handle all the zenoh message, one by one
            unsigned int len = _z_vec_len(&t_msg->body.frame.payload.messages);
            for (unsigned int i = 0; i < len; i++)
            {
                int res = _zn_handle_zenoh_message(ztu->session, (_zn_zenoh_message_t *)_z_vec_get(&t_msg->body.frame.payload.messages, i));
                if (res != _z_res_t_OK)
                    return res;
            }
            return _z_res_t_OK;
        }
    }

    default:
    {
        _Z_ERROR("Unknown session message ID\n");
        return _z_res_t_ERR;
    }
    }
}
