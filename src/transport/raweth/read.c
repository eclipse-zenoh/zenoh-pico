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

#include "zenoh-pico/transport/raweth/read.h"

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/transport/multicast/rx.h"
#include "zenoh-pico/transport/raweth/rx.h"
#include "zenoh-pico/transport/unicast/rx.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_RAWETH_TRANSPORT == 1

int8_t _zp_raweth_read(_z_transport_multicast_t *ztm) {
    int8_t ret = _Z_RES_OK;

    _z_slice_t addr;
    _z_transport_message_t t_msg;
    ret = _z_raweth_recv_t_msg(ztm, &t_msg, &addr);
    if (ret == _Z_RES_OK) {
        ret = _z_multicast_handle_transport_message(ztm, &t_msg, &addr);
        _z_t_msg_clear(&t_msg);
    }
    _z_slice_clear(&addr);
    return ret;
}
#else

int8_t _zp_raweth_read(_z_transport_multicast_t *ztm) {
    _ZP_UNUSED(ztm);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_RAWETH_TRANSPORT == 1

#if Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_RAWETH_TRANSPORT == 1

void *_zp_raweth_read_task(void *ztm_arg) {
    _z_transport_multicast_t *ztm = (_z_transport_multicast_t *)ztm_arg;
    _z_transport_message_t t_msg;
    _z_slice_t addr = _z_slice_wrap(NULL, 0);

    // Task loop
    while (ztm->_read_task_running == true) {
        // Read message from link
        int8_t ret = _z_raweth_recv_t_msg(ztm, &t_msg, &addr);
        switch (ret) {
            case _Z_RES_OK:
                // Process message
                break;
            case _Z_ERR_TRANSPORT_RX_FAILED:
                // Drop message
                _z_slice_clear(&addr);
                continue;
                break;
            default:
                // Drop message & stop task
                _Z_ERROR("Connection closed due to malformed message");
                ztm->_read_task_running = false;
                _z_slice_clear(&addr);
                continue;
                break;
        }
        // Process message
        if (_z_multicast_handle_transport_message(ztm, &t_msg, &addr) != _Z_RES_OK) {
            ztm->_read_task_running = false;
            _z_slice_clear(&addr);
            continue;
        }
        _z_t_msg_clear(&t_msg);
        _z_slice_clear(&addr);
    }
    return NULL;
}

int8_t _zp_raweth_start_read_task(_z_transport_t *zt, z_task_attr_t *attr, _z_task_t *task) {
    // Init memory
    (void)memset(task, 0, sizeof(_z_task_t));
    // Init task
    if (_z_task_init(task, attr, _zp_raweth_read_task, &zt->_transport._raweth) != _Z_RES_OK) {
        return _Z_ERR_SYSTEM_TASK_FAILED;
    }
    // Attach task
    zt->_transport._raweth._read_task = task;
    zt->_transport._raweth._read_task_running = true;
    return _Z_RES_OK;
}

int8_t _zp_raweth_stop_read_task(_z_transport_t *zt) {
    zt->_transport._raweth._read_task_running = false;
    return _Z_RES_OK;
}
#else

void *_zp_raweth_read_task(void *ztm_arg) {
    _ZP_UNUSED(ztm_arg);
    return NULL;
}
int8_t _zp_raweth_start_read_task(_z_transport_t *zt, void *attr, void *task) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(attr);
    _ZP_UNUSED(task);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _zp_raweth_stop_read_task(_z_transport_t *zt) {
    _ZP_UNUSED(zt);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif
