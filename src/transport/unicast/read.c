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

#include "zenoh-pico/transport/unicast/read.h"

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/transport/common/rx.h"
#include "zenoh-pico/transport/unicast/rx.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

int8_t _zp_unicast_read(_z_transport_unicast_t *ztu) {
    int8_t ret = _Z_RES_OK;

    _z_transport_message_t t_msg;
    ret = _z_unicast_recv_t_msg(ztu, &t_msg);
    if (ret == _Z_RES_OK) {
        ret = _z_unicast_handle_transport_message(ztu, &t_msg);
        _z_t_msg_clear(&t_msg);
    }

    return ret;
}
#else

int8_t _zp_unicast_read(_z_transport_unicast_t *ztu) {
    _ZP_UNUSED(ztu);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1

#if Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_UNICAST_TRANSPORT == 1

void *_zp_unicast_read_task(void *ztu_arg) {
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)ztu_arg;

    // Acquire and keep the lock
    _z_mutex_lock(&ztu->_mutex_rx);

    // Prepare the buffer
    _z_zbuf_reset(&ztu->_zbuf);

    while (ztu->_read_task_running == true) {
        // Read bytes from socket to the main buffer
        size_t to_read = 0;
        switch (ztu->_link._cap._flow) {
            case Z_LINK_CAP_FLOW_STREAM:
                if (_z_zbuf_len(&ztu->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                    _z_link_recv_zbuf(&ztu->_link, &ztu->_zbuf, NULL);
                    if (_z_zbuf_len(&ztu->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                        _z_zbuf_compact(&ztu->_zbuf);
                        continue;
                    }
                }
                // Get stream size
                to_read = _z_read_stream_size(&ztu->_zbuf);
                // Read data
                if (_z_zbuf_len(&ztu->_zbuf) < to_read) {
                    _z_link_recv_zbuf(&ztu->_link, &ztu->_zbuf, NULL);
                    if (_z_zbuf_len(&ztu->_zbuf) < to_read) {
                        _z_zbuf_set_rpos(&ztu->_zbuf, _z_zbuf_get_rpos(&ztu->_zbuf) - _Z_MSG_LEN_ENC_SIZE);
                        _z_zbuf_compact(&ztu->_zbuf);
                        continue;
                    }
                }
                break;
            case Z_LINK_CAP_FLOW_DATAGRAM:
                _z_zbuf_compact(&ztu->_zbuf);
                to_read = _z_link_recv_zbuf(&ztu->_link, &ztu->_zbuf, NULL);
                if (to_read == SIZE_MAX) {
                    continue;
                }
                break;
            default:
                break;
        }
        // Wrap the main buffer for to_read bytes
        _z_zbuf_t zbuf = _z_zbuf_view(&ztu->_zbuf, to_read);

        // Mark the session that we have received data
        ztu->_received = true;

        // Decode one session message
        _z_transport_message_t t_msg;
        int8_t ret = _z_transport_message_decode(&t_msg, &zbuf);

        if (ret == _Z_RES_OK) {
            ret = _z_unicast_handle_transport_message(ztu, &t_msg);
            if (ret == _Z_RES_OK) {
                _z_t_msg_clear(&t_msg);
            } else {
                ztu->_read_task_running = false;
                continue;
            }
        } else {
            _Z_ERROR("Connection closed due to malformed message");
            ztu->_read_task_running = false;
            continue;
        }

        // Move the read position of the read buffer
        _z_zbuf_set_rpos(&ztu->_zbuf, _z_zbuf_get_rpos(&ztu->_zbuf) + to_read);
    }
    _z_mutex_unlock(&ztu->_mutex_rx);
    return NULL;
}

int8_t _zp_unicast_start_read_task(_z_transport_t *zt, z_task_attr_t *attr, _z_task_t *task) {
    // Init memory
    (void)memset(task, 0, sizeof(_z_task_t));
    // Init task
    if (_z_task_init(task, attr, _zp_unicast_read_task, &zt->_transport._unicast) != _Z_RES_OK) {
        return _Z_ERR_SYSTEM_TASK_FAILED;
    }
    // Attach task
    zt->_transport._unicast._read_task = task;
    zt->_transport._unicast._read_task_running = true;
    return _Z_RES_OK;
}

int8_t _zp_unicast_stop_read_task(_z_transport_t *zt) {
    zt->_transport._unicast._read_task_running = false;
    return _Z_RES_OK;
}

#else

void *_zp_unicast_read_task(void *ztu_arg) {
    _ZP_UNUSED(ztu_arg);
    return NULL;
}

int8_t _zp_unicast_start_read_task(_z_transport_t *zt, void *attr, void *task) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(attr);
    _ZP_UNUSED(task);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _zp_unicast_stop_read_task(_z_transport_t *zt) {
    _ZP_UNUSED(zt);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif
