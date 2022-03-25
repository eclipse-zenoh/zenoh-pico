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
#include "zenoh-pico/utils/logging.h"

/*------------------ Reception helper ------------------*/
_z_transport_message_result_t _z_link_recv_t_msg(const _z_link_t *zl)
{
    _z_transport_message_result_t ret;

    // Create and prepare the buffer
    _z_zbuf_t zbf = _z_zbuf_make(Z_BATCH_SIZE);
    _z_zbuf_reset(&zbf);

    if (zl->is_streamed == 1)
    {
        // Read the message length
        if (_z_link_recv_exact_zbuf(zl, &zbf, _Z_MSG_LEN_ENC_SIZE, NULL) != _Z_MSG_LEN_ENC_SIZE)
            goto ERR;

        size_t len = 0;
        for (int i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++)
            len |= _z_zbuf_read(&zbf) << (i * 8);

        size_t writable = _z_zbuf_capacity(&zbf) - _z_zbuf_len(&zbf);
        if (writable < len)
            goto ERR;

        // Read enough bytes to decode the message
        if (_z_link_recv_exact_zbuf(zl, &zbf, len, NULL) != len)
            goto ERR;
    }
    else
    {
        if (_z_link_recv_zbuf(zl, &zbf, NULL) == SIZE_MAX)
            goto ERR;
    }

    _z_transport_message_result_t res = _z_transport_message_decode(&zbf);
    if (res.tag == _Z_RES_ERR)
        goto ERR;

    _z_t_msg_copy(&ret.value.transport_message, &res.value.transport_message);

    _z_zbuf_clear(&zbf);
    ret.tag = _Z_RES_OK;
    return ret;

ERR:
    _z_zbuf_clear(&zbf);

    ret.tag = _Z_RES_ERR;
    return ret;
}
