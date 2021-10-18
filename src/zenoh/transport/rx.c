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

#include "zenoh-pico/utils/private/logging.h"
#include "zenoh-pico/transport/private/utils.h"
#include "zenoh-pico/session/private/utils.h"

int _zn_handle_transport_message(zn_session_t *zn, _zn_transport_message_t *msg)
{
    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_SCOUT:
    {
        _Z_DEBUG("Handling of Scout messages not implemented");
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
        if (_ZN_HAS_FLAG(msg->header, _ZN_FLAG_T_A))
        {
            // The session lease
            zn->lease = msg->body.open.lease;

            // The initial SN at RX side. Initialize the session as we had already received
            // a message with a SN equal to initial_sn - 1.
            if (msg->body.open.initial_sn > 0)
            {
                zn->sn_rx_reliable = msg->body.open.initial_sn - 1;
                zn->sn_rx_best_effort = msg->body.open.initial_sn - 1;
            }
            else
            {
                zn->sn_rx_reliable = zn->sn_resolution - 1;
                zn->sn_rx_best_effort = zn->sn_resolution - 1;
            }
        }

        return _z_res_t_OK;
    }

    case _ZN_MID_CLOSE:
    {
        _Z_DEBUG("Closing session as requested by the remote peer");
        _zn_session_free(&zn);
        return _z_res_t_ERR;
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
        // Check if the SN is correct
        if (_ZN_HAS_FLAG(msg->header, _ZN_FLAG_T_R))
        {
            // @TODO: amend once reliability is in place. For the time being only
            //        monothonic SNs are ensured
            if (_zn_sn_precedes(zn->sn_resolution_half, zn->sn_rx_reliable, msg->body.frame.sn))
            {
                zn->sn_rx_reliable = msg->body.frame.sn;
            }
            else
            {
                _z_wbuf_reset(&zn->dbuf_reliable);
                _Z_DEBUG("Reliable message dropped because it is out of order");
                return _z_res_t_OK;
            }
        }
        else
        {
            if (_zn_sn_precedes(zn->sn_resolution_half, zn->sn_rx_best_effort, msg->body.frame.sn))
            {
                zn->sn_rx_best_effort = msg->body.frame.sn;
            }
            else
            {
                _z_wbuf_reset(&zn->dbuf_best_effort);
                _Z_DEBUG("Best effort message dropped because it is out of order");
                return _z_res_t_OK;
            }
        }

        if (_ZN_HAS_FLAG(msg->header, _ZN_FLAG_T_F))
        {
            int res = _z_res_t_OK;

            // Select the right defragmentation buffer
            _z_wbuf_t *dbuf = _ZN_HAS_FLAG(msg->header, _ZN_FLAG_T_R) ? &zn->dbuf_reliable : &zn->dbuf_best_effort;
            // Add the fragment to the defragmentation buffer
            _z_wbuf_add_iosli_from(dbuf, msg->body.frame.payload.fragment.val, msg->body.frame.payload.fragment.len);

            // Check if this is the last fragment
            if (_ZN_HAS_FLAG(msg->header, _ZN_FLAG_T_E))
            {
                // Convert the defragmentation buffer into a decoding buffer
                _z_zbuf_t zbf = _z_wbuf_to_zbuf(dbuf);

                // Decode the zenoh message
                _zn_zenoh_message_p_result_t r_zm = _zn_zenoh_message_decode(&zbf);
                if (r_zm.tag == _z_res_t_OK)
                {
                    _zn_zenoh_message_t *d_zm = r_zm.value.zenoh_message;
                    res = _zn_handle_zenoh_message(zn, d_zm);
                    // Free the decoded message
                    _zn_zenoh_message_free(d_zm);
                }
                else
                {
                    res = _z_res_t_ERR;
                }

                // Free the result
                _zn_zenoh_message_p_result_free(&r_zm);
                // Free the decoding buffer
                _z_zbuf_free(&zbf);
                // Reset the defragmentation buffer
                _z_wbuf_reset(dbuf);
            }

            return res;
        }
        else
        {
            // Handle all the zenoh message, one by one
            unsigned int len = z_vec_len(&msg->body.frame.payload.messages);
            for (unsigned int i = 0; i < len; ++i)
            {
                int res = _zn_handle_zenoh_message(zn, (_zn_zenoh_message_t *)z_vec_get(&msg->body.frame.payload.messages, i));
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
