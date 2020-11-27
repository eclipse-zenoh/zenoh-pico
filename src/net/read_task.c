/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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

#include <stdint.h>
#include "zenoh-pico/net/private/internal.h"
#include "zenoh-pico/net/private/msgcodec.h"
#include "zenoh-pico/net/private/system.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/private/logging.h"

void *_znp_read_task(void *arg)
{
    zn_session_t *z = (zn_session_t *)arg;
    z->read_task_running = 1;

    _zn_session_message_p_result_t r;
    _zn_session_message_p_result_init(&r);

    // Acquire and keep the lock
    _z_mutex_lock(&z->mutex_rx);
    // Prepare the buffer
    _z_rbuf_clear(&z->rbuf);
    while (z->read_task_running)
    {
#ifdef ZN_TRANSPORT_TCP_IP
        // NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
        //       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
        //       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
        //       the boundary of the serialized messages. The length is encoded as little-endian.
        //       In any case, the length of a message must not exceed 65_535 bytes.
        if (_z_rbuf_len(&z->rbuf) < _ZN_MSG_LEN_ENC_SIZE)
        {
            _z_rbuf_compact(&z->rbuf);
            // Read number of bytes to read
            while (_z_rbuf_len(&z->rbuf) < _ZN_MSG_LEN_ENC_SIZE)
            {
                if (_zn_recv_rbuf(z->sock, &z->rbuf) <= 0)
                    goto EXIT_RECV_LOOP;
            }
        }

        // Decode the message length
        size_t to_read = (size_t)((uint16_t)_z_rbuf_read(&z->rbuf) | ((uint16_t)_z_rbuf_read(&z->rbuf) << 8));

        if (_z_rbuf_len(&z->rbuf) < to_read)
        {
            _z_rbuf_compact(&z->rbuf);
            // Read the rest of bytes to decode one or more session messages
            while (_z_rbuf_len(&z->rbuf) < to_read)
            {
                if (_zn_recv_rbuf(z->sock, &z->rbuf) <= 0)
                    goto EXIT_RECV_LOOP;
            }
        }

        // Wrap the main buffer for to_read bytes
        _z_rbuf_t rbuf = _z_rbuf_view(&z->rbuf, to_read);
#else
        _z_rbuf_compact(&z->rbuf);

        // Read bytes from the socket.
        while (_z_rbuf_len(&z->rbuf) == 0)
        {
            if (_zn_recv_rbuf(z->sock, &z->rbuf) <= 0)
                goto EXIT_RECV_LOOP;
        }

        z_iobuf_t rbuf = z->rbuf;
#endif

        while (_z_rbuf_len(&rbuf) > 0)
        {
            // Mark the session that we have received data
            z->received = 1;

            // Decode one session message
            _zn_session_message_decode_na(&rbuf, &r);

            if (r.tag == _z_res_t_OK)
            {
                int res = _zn_handle_session_message(z, r.value.session_message);
                if (res == _z_res_t_OK)
                    _zn_session_message_free(r.value.session_message);
                else
                    goto EXIT_RECV_LOOP;
            }
            else
            {
                _Z_DEBUG("Connection closed due to malformed message");
                goto EXIT_RECV_LOOP;
            }
        }

#ifdef ZN_TRANSPORT_TCP_IP
        // Move the read position of the read buffer
        _z_rbuf_set_rpos(&z->rbuf, _z_rbuf_get_rpos(&z->rbuf) + to_read);
#endif
    }

EXIT_RECV_LOOP:
    if (z)
    {
        z->read_task_running = 0;
        // Release the lock
        _z_mutex_unlock(&z->mutex_rx);
    }

    // Free the result
    _zn_session_message_p_result_free(&r);

    return 0;
}

int znp_start_read_task(zn_session_t *z)
{
    _z_task_t *task = (_z_task_t *)malloc(sizeof(_z_task_t));
    memset(task, 0, sizeof(pthread_t));
    z->read_task = task;
    if (_z_task_init(task, NULL, _znp_read_task, z) != 0)
    {
        return -1;
    }
    return 0;
}

int znp_stop_read_task(zn_session_t *z)
{
    z->read_task_running = 0;
    return 0;
}
