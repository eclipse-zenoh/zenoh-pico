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
#include "zenoh-pico/utils/logging.h"

int _zp_unicast_read(_z_transport_unicast_t *ztu)
{
    _z_transport_message_result_t r_s = _z_unicast_recv_t_msg(ztu);
    if (r_s._tag == _Z_RES_ERR)
        goto ERR;

    int res = _z_unicast_handle_transport_message(ztu, &r_s._value._transport_message);
    _z_t_msg_clear(&r_s._value._transport_message);

    return res;

ERR:
    return _Z_RES_ERR;
}

void *_zp_unicast_read_task(void *arg)
{
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)arg;

    ztu->_read_task_running = 1;

    _z_transport_message_result_t r;

    // Acquire and keep the lock
    _z_mutex_lock(&ztu->_mutex_rx);

    // Prepare the buffer
    _z_zbuf_reset(&ztu->_zbuf);

    while (ztu->_read_task_running)
    {
        // Read bytes from socket to the main buffer
        size_t to_read = 0;
        if (ztu->_link->_is_streamed == 1)
        {
            if (_z_zbuf_len(&ztu->_zbuf) < _Z_MSG_LEN_ENC_SIZE)
            {
                _z_link_recv_zbuf(ztu->_link, &ztu->_zbuf, NULL);
                if (_z_zbuf_len(&ztu->_zbuf) < _Z_MSG_LEN_ENC_SIZE)
                    continue;
            }

            for (int i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++)
                to_read |= _z_zbuf_read(&ztu->_zbuf) << (i * 8);

            if (_z_zbuf_len(&ztu->_zbuf) < to_read)
            {
                _z_link_recv_zbuf(ztu->_link, &ztu->_zbuf, NULL);
                if (_z_zbuf_len(&ztu->_zbuf) < to_read)
                {
                    _z_zbuf_set_rpos(&ztu->_zbuf, _z_zbuf_get_rpos(&ztu->_zbuf) - _Z_MSG_LEN_ENC_SIZE);
                    continue;
                }
            }
        }
        else
        {
            to_read = _z_link_recv_zbuf(ztu->_link, &ztu->_zbuf, NULL);
            if (to_read == SIZE_MAX)
                continue;
        }

        // Wrap the main buffer for to_read bytes
        _z_zbuf_t zbuf = _z_zbuf_view(&ztu->_zbuf, to_read);

        // Mark the session that we have received data
        ztu->_received = 1;

        // Decode one session message
        _z_transport_message_decode_na(&zbuf, &r);

        if (r._tag == _Z_RES_OK)
        {
            int res = _z_unicast_handle_transport_message(ztu, &r._value._transport_message);
            if (res == _Z_RES_OK)
                _z_t_msg_clear(&r._value._transport_message);
            else
                goto EXIT_RECV_LOOP;
        }
        else
        {
            _Z_ERROR("Connection closed due to malformed message\n\n\n");
            goto EXIT_RECV_LOOP;
        }

        // Move the read position of the read buffer
        _z_zbuf_set_rpos(&ztu->_zbuf, _z_zbuf_get_rpos(&ztu->_zbuf) + to_read);
        _z_zbuf_compact(&ztu->_zbuf);
    }

EXIT_RECV_LOOP:
    if (ztu)
    {
        ztu->_read_task_running = 0;
        // Release the lock
        _z_mutex_unlock(&ztu->_mutex_rx);
    }

    return 0;
}
