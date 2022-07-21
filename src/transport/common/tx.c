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


/*------------------ Transmission helper ------------------*/
/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->mutex_tx
 */
void __unsafe_z_prepare_wbuf(_z_wbuf_t *buf, int is_streamed)
{
    _z_wbuf_reset(buf);

    if (is_streamed == 1)
    {
        for (size_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++)
            _z_wbuf_put(buf, 0, i);
        _z_wbuf_set_wpos(buf, _Z_MSG_LEN_ENC_SIZE);
    }
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->mutex_tx
 */
void __unsafe_z_finalize_wbuf(_z_wbuf_t *buf, int is_streamed)
{
    if (is_streamed == 1)
    {
        size_t len = _z_wbuf_len(buf) - _Z_MSG_LEN_ENC_SIZE;
        for (size_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++)
            _z_wbuf_put(buf, (uint8_t)((len >> 8 * i) & 0xFF), i);
    }
}

_z_transport_message_t _z_frame_header(z_reliability_t reliability, int is_fragment, int is_final, _z_zint_t sn)
{
    // Create the frame session message that carries the zenoh message
    int is_reliable = reliability == Z_RELIABILITY_RELIABLE;

    _z_transport_message_t t_msg = _z_t_msg_make_frame_header(sn, is_reliable, is_fragment, is_final);

    return t_msg;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->mutex_tx
 */
int __unsafe_z_serialize_zenoh_fragment(_z_wbuf_t *dst, _z_wbuf_t *src, z_reliability_t reliability, size_t sn)
{
    // Assume first that this is not the final fragment
    int is_final = 0;
    do
    {
        // Mark the buffer for the writing operation
        size_t w_pos = _z_wbuf_get_wpos(dst);
        // Get the frame header
        _z_transport_message_t f_hdr = _z_frame_header(reliability, 1, is_final, sn);
        // Encode the frame header
        int res = _z_transport_message_encode(dst, &f_hdr);
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
            return _z_wbuf_siphon(dst, src, to_copy);
        }
        else
        {
            return 0;
        }
    } while (1);
}

int _z_send_t_msg(_z_transport_t *zt, const _z_transport_message_t *t_msg)
{
    if (zt->_type == _Z_TRANSPORT_UNICAST_TYPE)
        return _z_unicast_send_t_msg(&zt->_transport._unicast, t_msg);
    else if (zt->_type == _Z_TRANSPORT_MULTICAST_TYPE)
        return _z_multicast_send_t_msg(&zt->_transport._multicast, t_msg);
    else
        return -1;
}

int _z_link_send_t_msg(const _z_link_t *zl, const _z_transport_message_t *t_msg)
{
    // Create and prepare the buffer to serialize the message on
    uint16_t mtu = zl->_mtu < Z_BATCH_SIZE_TX ? zl->_mtu : Z_BATCH_SIZE_TX;
    _z_wbuf_t wbf = _z_wbuf_make(mtu, 0);
    if (zl->_is_streamed == 1)
    {
        for (size_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++)
            _z_wbuf_put(&wbf, 0, i);
        _z_wbuf_set_wpos(&wbf, _Z_MSG_LEN_ENC_SIZE);
    }

    // Encode the session message
    if (_z_transport_message_encode(&wbf, t_msg) != 0)
        goto ERR;

    // Write the message legnth in the reserved space if needed
    if (zl->_is_streamed == 1)
    {
        size_t len = _z_wbuf_len(&wbf) - _Z_MSG_LEN_ENC_SIZE;
        for (size_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++)
            _z_wbuf_put(&wbf, (uint8_t)((len >> 8 * i) & 0xFF), i);
    }

    // Send the wbuf on the socket
    int res = _z_link_send_wbuf(zl, &wbf);

    // Release the buffer
    _z_wbuf_clear(&wbf);

    return res;

ERR:
    _z_wbuf_clear(&wbf);
    return -1;
}