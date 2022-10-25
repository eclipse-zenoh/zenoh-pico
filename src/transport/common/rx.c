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

#include "zenoh-pico/transport/link/rx.h"

#include <stddef.h>

#include "zenoh-pico/utils/logging.h"

/*------------------ Reception helper ------------------*/
_z_transport_message_result_t _z_link_recv_t_msg(const _z_link_t *zl) {
    _z_transport_message_result_t ret = {._tag = _Z_RES_OK};

    // Create and prepare the buffer
    _z_zbuf_t zbf = _z_zbuf_make(Z_BATCH_SIZE_RX);
    _z_zbuf_reset(&zbf);

    if (_Z_LINK_IS_STREAMED(zl->_capabilities) == true) {
        // Read the message length
        if (_z_link_recv_exact_zbuf(zl, &zbf, _Z_MSG_LEN_ENC_SIZE, NULL) == _Z_MSG_LEN_ENC_SIZE) {
            size_t len = 0;
            for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
                len |= (size_t)(_z_zbuf_read(&zbf) << (i * (uint8_t)8));
            }

            size_t writable = _z_zbuf_capacity(&zbf) - _z_zbuf_len(&zbf);
            if (writable >= len) {
                // Read enough bytes to decode the message
                if (_z_link_recv_exact_zbuf(zl, &zbf, len, NULL) != len) {
                    ret._tag = _Z_ERR_TRANSPORT_RX_FAILED;
                }
            } else {
                ret._tag = _Z_ERR_IOBUF_NO_SPACE;
            }
        } else {
            ret._tag = _Z_ERR_TRANSPORT_RX_FAILED;
        }
    } else {
        if (_z_link_recv_zbuf(zl, &zbf, NULL) == SIZE_MAX) {
            ret._tag = _Z_ERR_TRANSPORT_RX_FAILED;
        }
    }

    if (ret._tag == _Z_RES_OK) {
        _z_transport_message_result_t res = _z_transport_message_decode(&zbf);
        if (res._tag == _Z_RES_OK) {
            _z_t_msg_copy(&ret._value, &res._value);
        } else {
            ret._tag = res._tag;
        }
    }
    _z_zbuf_clear(&zbf);

    return ret;
}
