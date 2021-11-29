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

#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

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
    _z_wbuf_t wbf = _z_wbuf_make(ZN_WRITE_BUF_LEN, 0);
    if (zl->is_streamed == 1)
    {
        // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
        //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
        //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
        //       the boundary of the serialized messages. The length is encoded as little-endian.
        //       In any case, the length of a message must not exceed 65_535 bytes.
        for (size_t i = 0; i < _ZN_MSG_LEN_ENC_SIZE; i++)
            _z_wbuf_put(&wbf, 0, i);
        _z_wbuf_set_wpos(&wbf, _ZN_MSG_LEN_ENC_SIZE);
    }

    // Encode the session message
    if (_zn_transport_message_encode(&wbf, t_msg) != 0)
        goto ERR;

    // Write the message legnth in the reserved space if needed
    if (zl->is_streamed == 1)
    {
        size_t len = _z_wbuf_len(&wbf) - _ZN_MSG_LEN_ENC_SIZE;
        for (size_t i = 0; i < _ZN_MSG_LEN_ENC_SIZE; i++)
            _z_wbuf_put(&wbf, (uint8_t)((len >> 8 * i) & 0xFF), i);
    }

    // Send the wbuf on the socket
    int res = _zn_link_send_wbuf(zl, &wbf);

    // Release the buffer
    _z_wbuf_clear(&wbf);

    return res;

ERR:
    _z_wbuf_clear(&wbf);
    return -1;
}