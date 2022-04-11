//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include "zenoh-pico/transport/link/task/read.h"
#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/utils/logging.h"

int _znp_multicast_read(_zn_transport_multicast_t *ztm)
{
    z_bytes_t addr;
    _zn_transport_message_result_t r_s = _zn_multicast_recv_t_msg(ztm, &addr);
    if (r_s.tag == _z_res_t_ERR)
        goto ERR;

    int res = _zn_multicast_handle_transport_message(ztm, &r_s.value.transport_message, &addr);
    _zn_t_msg_clear(&r_s.value.transport_message);

    return res;

ERR:
    return _z_res_t_ERR;
}

void *_znp_multicast_read_task(void *arg)
{
    _zn_transport_multicast_t *ztm = (_zn_transport_multicast_t *)arg;

    ztm->read_task_running = 1;

    _zn_transport_message_result_t r;

    // Acquire and keep the lock
    z_mutex_lock(&ztm->mutex_rx);

    // Prepare the buffer
    _z_zbuf_reset(&ztm->zbuf);

    z_bytes_t addr;
    while (ztm->read_task_running)
    {
        // Read bytes from socket to the main buffer
        size_t to_read = 0;
        if (ztm->link->is_streamed == 1)
        {
            if (_z_zbuf_len(&ztm->zbuf) < _ZN_MSG_LEN_ENC_SIZE)
            {
                _zn_link_recv_zbuf(ztm->link, &ztm->zbuf, NULL);
                if (_z_zbuf_len(&ztm->zbuf) < _ZN_MSG_LEN_ENC_SIZE)
                    continue;
            }

            for (int i = 0; i < _ZN_MSG_LEN_ENC_SIZE; i++)
                to_read |= _z_zbuf_read(&ztm->zbuf) << (i * 8);

            if (_z_zbuf_len(&ztm->zbuf) < to_read)
            {
                _zn_link_recv_zbuf(ztm->link, &ztm->zbuf, NULL);
                if (_z_zbuf_len(&ztm->zbuf) < to_read)
                {
                    _z_zbuf_set_rpos(&ztm->zbuf, _z_zbuf_get_rpos(&ztm->zbuf) - _ZN_MSG_LEN_ENC_SIZE);
                    continue;
                }
            }
        }
        else
        {
            to_read = _zn_link_recv_zbuf(ztm->link, &ztm->zbuf, &addr);
            if (to_read == SIZE_MAX)
                continue;
        }

        // Wrap the main buffer for to_read bytes
        _z_zbuf_t zbuf = _z_zbuf_view(&ztm->zbuf, to_read);

        while (_z_zbuf_len(&zbuf) > 0)
        {
            // Decode one session message
            _zn_transport_message_decode_na(&zbuf, &r);

            if (r.tag == _z_res_t_OK)
            {
                int res = _zn_multicast_handle_transport_message(ztm, &r.value.transport_message, &addr);

                if (res == _z_res_t_OK)
                {
                    _zn_t_msg_clear(&r.value.transport_message);
                    _z_bytes_clear(&addr);
                }
                else
                    goto EXIT_RECV_LOOP;
            }
            else
            {
                _Z_ERROR("Connection closed due to malformed message\n");
                goto EXIT_RECV_LOOP;
            }
        }

        // Move the read position of the read buffer
        _z_zbuf_set_rpos(&ztm->zbuf, _z_zbuf_get_rpos(&ztm->zbuf) + to_read);
        _z_zbuf_compact(&ztm->zbuf);
    }

EXIT_RECV_LOOP:
    if (ztm)
    {
        ztm->read_task_running = 0;
        // Release the lock
        z_mutex_unlock(&ztm->mutex_rx);
    }

    return 0;
}
