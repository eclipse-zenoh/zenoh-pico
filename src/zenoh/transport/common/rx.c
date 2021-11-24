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

#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/transport/link/rx.h"

/*------------------ Reception helper ------------------*/
_zn_transport_message_p_result_t _zn_recv_t_msg(_zn_transport_t *zt)
{
    _zn_transport_message_p_result_t r;
    _zn_transport_message_p_result_init(&r);

    if (zt->type == _ZN_TRANSPORT_UNICAST_TYPE)
        _zn_unicast_recv_t_msg_na(&zt->transport.unicast, &r);
    else if (zt->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        _zn_multicast_recv_t_msg_na(&zt->transport.multicast, &r);

    return r;
}

_zn_transport_message_p_result_t _zn_recv_t_msg_nt(const _zn_link_t *zl)
{
    _zn_transport_message_p_result_t ret;

    // Create and prepare the buffer
    _z_zbuf_t zbf = _z_zbuf_make(ZN_READ_BUF_LEN);
    _z_zbuf_clear(&zbf);

    if (zl->is_streamed == 1)
    {
        // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
        //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
        //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
        //       the boundary of the serialized messages. The length is encoded as little-endian.
        //       In any case, the length of a message must not exceed 65_535 bytes.

        // Read the message length
        if (_zn_link_recv_exact_zbuf(zl, &zbf, _ZN_MSG_LEN_ENC_SIZE) != _ZN_MSG_LEN_ENC_SIZE)
            goto ERR;

        uint16_t len = _z_zbuf_read(&zbf) | (_z_zbuf_read(&zbf) << 8);
        size_t writable = _z_zbuf_capacity(&zbf) - _z_zbuf_len(&zbf);
        if (writable < len)
            goto ERR;

        // Read enough bytes to decode the message
        if (_zn_link_recv_exact_zbuf(zl, &zbf, len) != len)
            goto ERR;
    }
    else
    {
        if (_zn_link_recv_zbuf(zl, &zbf) < 0)
            goto ERR;
    }

    ret = _zn_transport_message_decode(&zbf);

    _z_zbuf_free(&zbf);

    return ret;

ERR:
    _z_zbuf_free(&zbf);

    ret.tag = _z_res_t_ERR;
    return ret;
}

void _zn_recv_t_msg_na(_zn_transport_t *zt, _zn_transport_message_p_result_t *r)
{
    if (zt->type == _ZN_TRANSPORT_UNICAST_TYPE)
        _zn_unicast_recv_t_msg_na(&zt->transport.unicast, r);
    else if (zt->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        _zn_multicast_recv_t_msg_na(&zt->transport.multicast, r);
}

int _zn_handle_transport_message(_zn_transport_t *zt, _zn_transport_message_t *msg)
{
    if (zt->type == _ZN_TRANSPORT_UNICAST_TYPE)
        return _zn_unicast_handle_transport_message(&zt->transport.unicast, msg);
    else if (zt->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        return _zn_multicast_handle_transport_message(&zt->transport.multicast, msg);
    else
        return -1;
}
