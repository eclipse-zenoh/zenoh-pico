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
_zn_transport_message_result_t _zn_link_recv_t_msg(const _zn_link_t *zl)
{
    _zn_transport_message_result_t ret;

    // Create and prepare the buffer
    _z_zbuf_t zbf = _z_zbuf_make(ZN_BATCH_SIZE);
    _z_zbuf_reset(&zbf);

    if (zl->is_streamed == 1)
    {
        // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
        //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
        //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
        //       the boundary of the serialized messages. The length is encoded as little-endian.
        //       In any case, the length of a message must not exceed 65_535 bytes.

        // Read the message length
        if (_zn_link_recv_exact_zbuf(zl, &zbf, _ZN_MSG_LEN_ENC_SIZE, NULL) != _ZN_MSG_LEN_ENC_SIZE)
            goto ERR;

        uint16_t len = _z_zbuf_read(&zbf) | (_z_zbuf_read(&zbf) << 8);
        size_t writable = _z_zbuf_capacity(&zbf) - _z_zbuf_len(&zbf);
        if (writable < len)
            goto ERR;

        // Read enough bytes to decode the message
        if (_zn_link_recv_exact_zbuf(zl, &zbf, len, NULL) != len)
            goto ERR;
    }
    else
    {
        if (_zn_link_recv_zbuf(zl, &zbf, NULL) < 0)
            goto ERR;
    }

    _zn_transport_message_result_t res = _zn_transport_message_decode(&zbf);
    if (res.tag == _z_res_t_ERR)
        return ret;

    _zn_t_msg_copy(&ret.value.transport_message, &res.value.transport_message);

    _z_zbuf_clear(&zbf);
    ret.tag = _z_res_t_OK;
    return ret;

ERR:
    _z_zbuf_clear(&zbf);

    ret.tag = _z_res_t_ERR;
    return ret;
}
