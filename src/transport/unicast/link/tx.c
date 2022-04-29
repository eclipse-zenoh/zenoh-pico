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

#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ SN helper ------------------*/
/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->_mutex_inner
 */
_z_zint_t __unsafe_z_unicast_get_sn(_z_transport_unicast_t *ztu, z_reliability_t reliability)
{
    _z_zint_t sn;
    // Get the sequence number and update it in modulo operation
    if (reliability == Z_RELIABILITY_RELIABLE)
    {
        sn = ztu->_sn_tx_reliable;
        ztu->_sn_tx_reliable = (ztu->_sn_tx_reliable + 1) % ztu->_sn_resolution;
    }
    else
    {
        sn = ztu->_sn_tx_best_effort;
        ztu->_sn_tx_best_effort = (ztu->_sn_tx_best_effort + 1) % ztu->_sn_resolution;
    }
    return sn;
}

int _z_unicast_send_t_msg(_z_transport_unicast_t *ztu, const _z_transport_message_t *t_msg)
{
    _Z_DEBUG(">> send session message\n");

    // Acquire the lock
    _z_mutex_lock(&ztu->_mutex_tx);

    // Prepare the buffer eventually reserving space for the message length
    __unsafe_z_prepare_wbuf(&ztu->_wbuf, ztu->_link->_is_streamed);

    // Encode the session message
    int res = _z_transport_message_encode(&ztu->_wbuf, t_msg);
    if (res == 0)
    {
        // Write the message legnth in the reserved space if needed
        __unsafe_z_finalize_wbuf(&ztu->_wbuf, ztu->_link->_is_streamed);
        // Send the wbuf on the socket
        res = _z_link_send_wbuf(ztu->_link, &ztu->_wbuf);
        // Mark the session that we have transmitted data
        ztu->_transmitted = 1;
    }
    else
    {
        _Z_INFO("Dropping session message because it is too large\n");
    }

    // Release the lock
    _z_mutex_unlock(&ztu->_mutex_tx);

    return res;
}

int _z_unicast_send_z_msg(_z_session_t *zn, _z_zenoh_message_t *z_msg, z_reliability_t reliability, _z_congestion_control_t cong_ctrl)
{
    _Z_DEBUG(">> send zenoh message\n");

    _z_transport_unicast_t *ztu = &zn->_tp->_transport._unicast;

    // Acquire the lock and drop the message if needed
    if (cong_ctrl == Z_CONGESTION_CONTROL_BLOCK)
    {
        _z_mutex_lock(&ztu->_mutex_tx);
    }
    else
    {
        int locked = _z_mutex_trylock(&ztu->_mutex_tx);
        if (locked != 0)
        {
            _Z_INFO("Dropping zenoh message because of congestion control\n");
            // We failed to acquire the lock, drop the message
            return 0;
        }
    }

    // Prepare the buffer eventually reserving space for the message length
    __unsafe_z_prepare_wbuf(&ztu->_wbuf, ztu->_link->_is_streamed);

    // Get the next sequence number
    _z_zint_t sn = __unsafe_z_unicast_get_sn(ztu, reliability);
    // Create the frame header that carries the zenoh message
    _z_transport_message_t t_msg = _z_frame_header(reliability, 0, 0, sn);

    // Encode the frame header
    int res = _z_transport_message_encode(&ztu->_wbuf, &t_msg);
    if (res != 0)
    {
        _Z_INFO("Dropping zenoh message because the session frame can not be encoded\n");
        goto EXIT_ZSND_PROC;
    }

    // Encode the zenoh message
    res = _z_zenoh_message_encode(&ztu->_wbuf, z_msg);
    if (res == 0)
    {
        // Write the message legnth in the reserved space if needed
        __unsafe_z_finalize_wbuf(&ztu->_wbuf, ztu->_link->_is_streamed);

        // Send the wbuf on the socket
        res = _z_link_send_wbuf(ztu->_link, &ztu->_wbuf);
        if (res == 0)
            ztu->_transmitted = 1;
    }
    else
    {
        // The message does not fit in the current batch, let's fragment it
        // Create an expandable wbuf for fragmentation
        _z_wbuf_t fbf = _z_wbuf_make(Z_IOSLICE_SIZE, 1);

        // Encode the message on the expandable wbuf
        res = _z_zenoh_message_encode(&fbf, z_msg);
        if (res != 0)
        {
            _Z_INFO("Dropping zenoh message because it can not be fragmented\n");
            goto EXIT_FRAG_PROC;
        }

        // Fragment and send the message
        int is_first = 1;
        while (_z_wbuf_len(&fbf) > 0)
        {
            // Get the fragment sequence number
            if (!is_first)
                sn = __unsafe_z_unicast_get_sn(ztu, reliability);
            is_first = 0;

            // Clear the buffer for serialization
            __unsafe_z_prepare_wbuf(&ztu->_wbuf, ztu->_link->_is_streamed);

            // Serialize one fragment
            res = __unsafe_z_serialize_zenoh_fragment(&ztu->_wbuf, &fbf, reliability, sn);
            if (res != 0)
            {
                _Z_INFO("Dropping zenoh message because it can not be fragmented\n");
                goto EXIT_FRAG_PROC;
            }

            // Write the message length in the reserved space if needed
            __unsafe_z_finalize_wbuf(&ztu->_wbuf, ztu->_link->_is_streamed);

            // Send the wbuf on the socket
            res = _z_link_send_wbuf(ztu->_link, &ztu->_wbuf);
            if (res != 0)
            {
                _Z_INFO("Dropping zenoh message because it can not sent\n");
                goto EXIT_FRAG_PROC;
            }

            // Mark the session that we have transmitted data
            ztu->_transmitted = 1;
        }

    EXIT_FRAG_PROC:
        // Free the fragmentation buffer memory
        _z_wbuf_clear(&fbf);
    }

EXIT_ZSND_PROC:
    // Release the lock
    _z_mutex_unlock(&ztu->_mutex_tx);

    return res;
}