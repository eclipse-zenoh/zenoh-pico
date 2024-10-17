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

#include "zenoh-pico/transport/multicast/read.h"

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/transport/common/rx.h"
#include "zenoh-pico/transport/multicast/rx.h"
#include "zenoh-pico/transport/unicast/rx.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_MULTICAST_TRANSPORT == 1

z_result_t _zp_multicast_read(_z_transport_multicast_t *ztm) {
    z_result_t ret = _Z_RES_OK;

    _z_slice_t addr;
    _z_transport_message_t t_msg;
    ret = _z_multicast_recv_t_msg(ztm, &t_msg, &addr);
    if (ret == _Z_RES_OK) {
        ret = _z_multicast_handle_transport_message(ztm, &t_msg, &addr);
        _z_t_msg_clear(&t_msg);
    }

    return ret;
}
#else
z_result_t _zp_multicast_read(_z_transport_multicast_t *ztm) {
    _ZP_UNUSED(ztm);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1

#if Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_MULTICAST_TRANSPORT == 1

void *_zp_multicast_read_task(void *ztm_arg) {
    _z_transport_multicast_t *ztm = (_z_transport_multicast_t *)ztm_arg;

    // Acquire and keep the lock
    _z_mutex_lock(&ztm->_mutex_rx);

    // Prepare the buffer
    _z_zbuf_reset(&ztm->_zbuf);

    _z_slice_t addr = _z_slice_empty();
    while (ztm->_read_task_running == true) {
        size_t to_read = 0;

        // Read bytes from socket to the main buffer
        switch (ztm->_link._cap._flow) {
            case Z_LINK_CAP_FLOW_STREAM:
                if (_z_zbuf_len(&ztm->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                    _z_link_recv_zbuf(&ztm->_link, &ztm->_zbuf, &addr);
                    if (_z_zbuf_len(&ztm->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                        _z_slice_clear(&addr);
                        _z_zbuf_compact(&ztm->_zbuf);
                        continue;
                    }
                }
                // Get stream size
                to_read = _z_read_stream_size(&ztm->_zbuf);
                // Read data
                if (_z_zbuf_len(&ztm->_zbuf) < to_read) {
                    _z_link_recv_zbuf(&ztm->_link, &ztm->_zbuf, NULL);
                    if (_z_zbuf_len(&ztm->_zbuf) < to_read) {
                        _z_zbuf_set_rpos(&ztm->_zbuf, _z_zbuf_get_rpos(&ztm->_zbuf) - _Z_MSG_LEN_ENC_SIZE);
                        _z_zbuf_compact(&ztm->_zbuf);
                        continue;
                    }
                }
                break;
            case Z_LINK_CAP_FLOW_DATAGRAM:
                _z_zbuf_compact(&ztm->_zbuf);
                to_read = _z_link_recv_zbuf(&ztm->_link, &ztm->_zbuf, &addr);
                if (to_read == SIZE_MAX) {
                    continue;
                }
                break;
            default:
                break;
        }
        // Wrap the main buffer to_read bytes
        _z_zbuf_t zbuf = _z_zbuf_view(&ztm->_zbuf, to_read);

        while (_z_zbuf_len(&zbuf) > 0) {
            // Decode one session message
            _z_transport_message_t t_msg;
            z_result_t ret = _z_transport_message_decode(&t_msg, &zbuf);
            if (ret == _Z_RES_OK) {
                ret = _z_multicast_handle_transport_message(ztm, &t_msg, &addr);

                if (ret == _Z_RES_OK) {
                    _z_t_msg_clear(&t_msg);
                    _z_slice_clear(&addr);
                } else {
                    _Z_ERROR("Dropping message due to processing error: %d", ret);
                    continue;
                }
            } else {
                _Z_ERROR("Connection closed due to malformed message: %d", ret);
                ztm->_read_task_running = false;
                continue;
            }
        }
        // Move the read position of the read buffer
        _z_zbuf_set_rpos(&ztm->_zbuf, _z_zbuf_get_rpos(&ztm->_zbuf) + to_read);
        // Check if user or defragment buffer took ownership of buffer
        if (!_z_zbuf_is_last_ref(&ztm->_zbuf)) {
            // Allocate a new buffer
            _z_zbuf_t new_zbuf = _z_zbuf_make(Z_BATCH_MULTICAST_SIZE);
            if (_z_zbuf_capacity(&new_zbuf) != Z_BATCH_MULTICAST_SIZE) {
                _Z_ERROR("Connection closed due to lack of memory to allocate rx buffer");
                ztm->_read_task_running = false;
            }
            // Recopy leftover bytes
            size_t leftovers = _z_zbuf_len(&ztm->_zbuf);
            if (leftovers > 0) {
                _z_zbuf_copy_bytes(&new_zbuf, &ztm->_zbuf);
            }
            // Drop buffer & update
            _z_zbuf_clear(&ztm->_zbuf);
            ztm->_zbuf = new_zbuf;
        }
    }
    _z_mutex_unlock(&ztm->_mutex_rx);
    return NULL;
}

z_result_t _zp_multicast_start_read_task(_z_transport_t *zt, z_task_attr_t *attr, _z_task_t *task) {
    // Init memory
    (void)memset(task, 0, sizeof(_z_task_t));
    zt->_transport._multicast._read_task_running = true;  // Init before z_task_init for concurrency issue
    // Init task
    if (_z_task_init(task, attr, _zp_multicast_read_task, &zt->_transport._multicast) != _Z_RES_OK) {
        zt->_transport._multicast._read_task_running = false;
        return _Z_ERR_SYSTEM_TASK_FAILED;
    }
    // Attach task
    zt->_transport._multicast._read_task = task;
    return _Z_RES_OK;
}

z_result_t _zp_multicast_stop_read_task(_z_transport_t *zt) {
    zt->_transport._multicast._read_task_running = false;
    return _Z_RES_OK;
}
#else

void *_zp_multicast_read_task(void *ztm_arg) {
    _ZP_UNUSED(ztm_arg);
    return NULL;
}

z_result_t _zp_multicast_start_read_task(_z_transport_t *zt, void *attr, void *task) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(attr);
    _ZP_UNUSED(task);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _zp_multicast_stop_read_task(_z_transport_t *zt) {
    _ZP_UNUSED(zt);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif
