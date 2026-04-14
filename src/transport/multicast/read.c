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
#include "zenoh-pico/runtime/runtime.h"
#include "zenoh-pico/transport/common/rx.h"
#include "zenoh-pico/transport/multicast/lease.h"
#include "zenoh-pico/transport/multicast/rx.h"
#include "zenoh-pico/transport/unicast/rx.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_MULTICAST_TRANSPORT == 1

static z_result_t _zp_multicast_process_messages(_z_transport_multicast_t *ztm) {
    size_t to_read = 0;

    z_result_t ret = _z_multicast_recv_zbuf(ztm, &to_read);
    if (ret == _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES || ret == _Z_ERR_TRANSPORT_RX_FAILED) {
        return _Z_NO_DATA_PROCESSED;
    } else if (ret != _Z_RES_OK) {
        return ret;
    }

    // Wrap the main buffer to_read bytes
    _z_zbuf_t zbuf = _z_zbuf_view(&ztm->_common._zbuf, to_read);
    while (_z_zbuf_len(&zbuf) > 0) {
        // Decode one session message
        _z_transport_message_t t_msg;
        ret = _z_transport_message_decode(&t_msg, &zbuf);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Connection closed due to malformed message: %d", ret);
            break;
        }

        ret = _z_multicast_handle_transport_message(ztm, &t_msg, &ztm->_zbuf_addr);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Dropping message due to processing error: %d", ret);
            break;
        }
    }
    // Move the read position of the read buffer
    _z_zbuf_set_rpos(&ztm->_common._zbuf, _z_zbuf_get_rpos(&ztm->_common._zbuf) + to_read);
    if (_z_multicast_update_rx_buffer(ztm) != _Z_RES_OK) {
        _Z_ERROR("Connection closed due to lack of memory to allocate rx buffer");
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    return ret;
}

z_result_t _zp_multicast_read(_z_transport_multicast_t *ztm, bool single_read) {
    // Read & process a single message
    if (single_read) {
        _z_transport_message_t t_msg;
        _Z_RETURN_IF_ERR(_z_multicast_recv_t_msg(ztm, &t_msg));
        _Z_CLEAN_RETURN_IF_ERR(_z_multicast_handle_transport_message(ztm, &t_msg, &ztm->_zbuf_addr),
                               _z_t_msg_clear(&t_msg));
        _z_t_msg_clear(&t_msg);
        return _z_multicast_update_rx_buffer(ztm);
    } else {
        return _zp_multicast_process_messages(ztm);
    }
}

_z_fut_fn_result_t _zp_multicast_read_task_fn(void *ztm_arg, _z_executor_t *executor) {
    _z_transport_multicast_t *ztm = (_z_transport_multicast_t *)ztm_arg;
    if (ztm->_common._state == _Z_TRANSPORT_STATE_CLOSED) {
        return _z_fut_fn_result_ready();
    } else if (ztm->_common._state == _Z_TRANSPORT_STATE_RECONNECTING) {
        return _z_fut_fn_result_suspend();
    }
    if (_zp_multicast_process_messages(ztm) < _Z_RES_OK) {
        // TODO: report failure and disconnect ?
        _Z_WARN("Multicast read task failed");
        return _zp_multicast_failed_result(ztm, executor);
    } else {
        return _z_fut_fn_result_continue();
    }
}
#endif
