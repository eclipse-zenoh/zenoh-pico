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

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/utils/logging.h"

#if Z_MULTICAST_TRANSPORT == 1

int8_t _zp_multicast_read(_z_transport_multicast_t *ztm) {
    int8_t ret = _Z_RES_OK;

    _z_bytes_t addr;
    _z_transport_message_result_t r_s = _z_multicast_recv_t_msg(ztm, &addr);
    if (r_s._tag == _Z_RES_OK) {
        ret = _z_multicast_handle_transport_message(ztm, &r_s._value, &addr);
        _z_t_msg_clear(&r_s._value);
    } else {
        ret = r_s._tag;
    }

    return ret;
}

void *_zp_multicast_read_task(void *ztm_arg) {
#if Z_MULTI_THREAD == 1
    _z_transport_multicast_t *ztm = (_z_transport_multicast_t *)ztm_arg;

    ztm->_read_task_running = 1;

    _z_transport_message_result_t r;

    // Acquire and keep the lock
    _z_mutex_lock(&ztm->_mutex_rx);

    // Prepare the buffer
    _z_zbuf_reset(&ztm->_zbuf);

    _z_bytes_t addr = _z_bytes_wrap(NULL, 0);
    while (ztm->_read_task_running == 1) {
        // Read bytes from socket to the main buffer
        size_t to_read = 0;
        if (_Z_LINK_IS_STREAMED(ztm->_link->_capabilities) != 0) {
            if (_z_zbuf_len(&ztm->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                _z_link_recv_zbuf(ztm->_link, &ztm->_zbuf, &addr);
                if (_z_zbuf_len(&ztm->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                    _z_bytes_clear(&addr);
                    continue;
                }
            }

            for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
                to_read |= _z_zbuf_read(&ztm->_zbuf) << (i * (uint8_t)8);
            }

            if (_z_zbuf_len(&ztm->_zbuf) < to_read) {
                _z_link_recv_zbuf(ztm->_link, &ztm->_zbuf, NULL);
                if (_z_zbuf_len(&ztm->_zbuf) < to_read) {
                    _z_zbuf_set_rpos(&ztm->_zbuf, _z_zbuf_get_rpos(&ztm->_zbuf) - _Z_MSG_LEN_ENC_SIZE);
                    continue;
                }
            }
        } else {
            to_read = _z_link_recv_zbuf(ztm->_link, &ztm->_zbuf, &addr);
            if (to_read == SIZE_MAX) {
                continue;
            }
        }

        // Wrap the main buffer for to_read bytes
        _z_zbuf_t zbuf = _z_zbuf_view(&ztm->_zbuf, to_read);

        while (_z_zbuf_len(&zbuf) > 0) {
            // Decode one session message
            _z_transport_message_decode_na(&zbuf, &r);

            if (r._tag == _Z_RES_OK) {
                int res = _z_multicast_handle_transport_message(ztm, &r._value, &addr);

                if (res == _Z_RES_OK) {
                    _z_t_msg_clear(&r._value);
                    _z_bytes_clear(&addr);
                } else {
                    ztm->_read_task_running = 0;
                    continue;
                }
            } else {
                _Z_ERROR("Connection closed due to malformed message\n");
                ztm->_read_task_running = 0;
                continue;
            }
        }

        // Move the read position of the read buffer
        _z_zbuf_set_rpos(&ztm->_zbuf, _z_zbuf_get_rpos(&ztm->_zbuf) + to_read);
        _z_zbuf_compact(&ztm->_zbuf);
    }

    _z_mutex_unlock(&ztm->_mutex_rx);
#endif  // Z_MULTI_THREAD == 1

    return 0;
}

#endif  // Z_MULTICAST_TRANSPORT == 1
