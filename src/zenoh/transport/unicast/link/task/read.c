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

#include "zenoh-pico/transport/link/task/read.h"
#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

int _znp_unicast_read(_zn_transport_unicast_t *ztu)
{
    _zn_transport_message_result_t r_s = _zn_unicast_recv_t_msg(ztu);
    if (r_s.tag == _z_res_t_OK)
    {
        int res = _zn_unicast_handle_transport_message(ztu, &r_s.value.transport_message);
        _zn_transport_message_free(&r_s.value.transport_message);

        return res;
    }
    else
        return _z_res_t_ERR;
}

void *_znp_unicast_read_task(void *arg)
{
    _zn_transport_unicast_t *ztu = (_zn_transport_unicast_t *)arg;

    ztu->read_task_running = 1;

    _zn_transport_message_result_t r;

    // Acquire and keep the lock
    z_mutex_lock(&ztu->mutex_rx);
    // Prepare the buffer
    _z_zbuf_clear(&ztu->zbuf);
    while (ztu->read_task_running)
    {
        size_t to_read = 0;
        if (ztu->link->is_streamed == 1)
        {
            // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
            //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
            //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
            //       the boundary of the serialized messages. The length is encoded as little-endian.
            //       In any case, the length of a message must not exceed 65_535 bytes.
            if (_z_zbuf_len(&ztu->zbuf) < _ZN_MSG_LEN_ENC_SIZE)
            {
                _z_zbuf_compact(&ztu->zbuf);
                // Read number of bytes to read
                while (_z_zbuf_len(&ztu->zbuf) < _ZN_MSG_LEN_ENC_SIZE)
                {
                    if (_zn_link_recv_zbuf(ztu->link, &ztu->zbuf) <= 0)
                        goto EXIT_RECV_LOOP;
                }
            }

            // Decode the message length
            to_read = (size_t)((uint16_t)_z_zbuf_read(&ztu->zbuf) | ((uint16_t)_z_zbuf_read(&ztu->zbuf) << 8));

            if (_z_zbuf_len(&ztu->zbuf) < to_read)
            {
                _z_zbuf_compact(&ztu->zbuf);
                // Read the rest of bytes to decode one or more session messages
                while (_z_zbuf_len(&ztu->zbuf) < to_read)
                {
                    if (_zn_link_recv_zbuf(ztu->link, &ztu->zbuf) <= 0)
                        goto EXIT_RECV_LOOP;
                }
            }
        }
        else
        {
            _z_zbuf_compact(&ztu->zbuf);

            // Read bytes from the socket
            to_read = _zn_link_recv_zbuf(ztu->link, &ztu->zbuf);
            if (to_read == -1)
                continue;
        }

        // Wrap the main buffer for to_read bytes
        _z_zbuf_t zbuf = _z_zbuf_view(&ztu->zbuf, to_read);

        while (_z_zbuf_len(&zbuf) > 0)
        {
            // Mark the session that we have received data
            ztu->received = 1;

            // Decode one session message
            _zn_transport_message_decode_na(&zbuf, &r);

            if (r.tag == _z_res_t_OK)
            {
                int res = _zn_unicast_handle_transport_message(ztu, &r.value.transport_message);
                if (res == _z_res_t_OK)
                    _zn_transport_message_free(&r.value.transport_message);
                else
                    goto EXIT_RECV_LOOP;
            }
            else
            {
                _Z_DEBUG("Connection closed due to malformed message");
                goto EXIT_RECV_LOOP;
            }
        }

        // Move the read position of the read buffer
        _z_zbuf_set_rpos(&ztu->zbuf, _z_zbuf_get_rpos(&ztu->zbuf) + to_read);
    }

EXIT_RECV_LOOP:
    if (ztu)
    {
        ztu->read_task_running = 0;
        // Release the lock
        z_mutex_unlock(&ztu->mutex_rx);
    }

    return 0;
}