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
void __unsafe_zn_prepare_wbuf(_z_wbuf_t *buf, int is_streamed)
{
    _z_wbuf_reset(buf);

    if (is_streamed == 1)
    {
        // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
        //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
        //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
        //       the boundary of the serialized messages. The length is encoded as little-endian.
        //       In any case, the length of a message must not exceed 65_535 bytes.
        for (size_t i = 0; i < _ZN_MSG_LEN_ENC_SIZE; i++)
            _z_wbuf_put(buf, 0, i);
        _z_wbuf_set_wpos(buf, _ZN_MSG_LEN_ENC_SIZE);
    }
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - ztu->mutex_tx
 */
void __unsafe_zn_finalize_wbuf(_z_wbuf_t *buf, int is_streamed)
{
    if (is_streamed == 1)
    {
        // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
        //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
        //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
        //       the boundary of the serialized messages. The length is encoded as little-endian.
        //       In any case, the length of a message must not exceed 65_535 bytes.
        size_t len = _z_wbuf_len(buf) - _ZN_MSG_LEN_ENC_SIZE;
        for (size_t i = 0; i < _ZN_MSG_LEN_ENC_SIZE; i++)
            _z_wbuf_put(buf, (uint8_t)((len >> 8 * i) & 0xFF), i);
    }
}

int _zn_serialize_zenoh_fragment(_z_wbuf_t *dst, _z_wbuf_t *src, zn_reliability_t reliability, size_t sn)
{
    uint8_t is_final = 0;
    do
    {
        size_t w_pos = _z_wbuf_get_wpos(dst);

        // Encode the fragment
        _zn_transport_message_t f_hdr = _zn_t_msg_make_fragment(reliability, is_final, sn, _z_bytes_make(0));
        int res = _zn_transport_message_encode(dst, &f_hdr);
        if (res != 0)
            return -1;

        size_t space_left = _z_wbuf_space_left(dst);
        size_t bytes_left = _z_wbuf_len(src);

        // Check if it is really the final fragment
        if (bytes_left <= space_left && !is_final)
        {
            // It is really the finally fragment, reserialize the header
            _z_wbuf_set_wpos(dst, w_pos);
            is_final = 1;
            continue;
        }

        // Write the fragment
        size_t to_copy = bytes_left <= space_left ? bytes_left : space_left;
        return _z_wbuf_siphon(dst, src, to_copy);

    } while (1);
}

int _zn_send_t_msg(_zn_transport_t *zt, const _zn_transport_message_t *t_msg)
{
    if (zt->type == _ZN_TRANSPORT_UNICAST_TYPE)
        return _zn_unicast_send_t_msg(&zt->transport.unicast, t_msg);
    else if (zt->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        return _zn_multicast_send_t_msg(&zt->transport.multicast, t_msg);
    else
        return -1;
}

int _zn_link_send_t_msg(const _zn_link_t *zl, const _zn_transport_message_t *t_msg)
{
    // Create and prepare the buffer to serialize the message on
    uint16_t mtu = zl->mtu < ZN_BATCH_SIZE ? zl->mtu : ZN_BATCH_SIZE;
    _z_wbuf_t wbf = _z_wbuf_make(mtu, 0);
    __unsafe_zn_prepare_wbuf(&wbf, zl->is_streamed);

    // Encode the session message
    if (_zn_transport_message_encode(&wbf, t_msg) != 0)
        goto ERR;

    // Write the message legnth in the reserved space if needed
    __unsafe_zn_finalize_wbuf(&wbf, zl->is_streamed);

    // Send the wbuf on the socket
    int res = _zn_link_send_wbuf(zl, &wbf);

    // Release the buffer
    _z_wbuf_clear(&wbf);

    return res;

ERR:
    _z_wbuf_clear(&wbf);
    return -1;
}