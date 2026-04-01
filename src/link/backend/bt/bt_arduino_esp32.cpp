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

#include "zenoh-pico/config.h"

#if defined(ZENOH_ARDUINO_ESP32) && Z_FEATURE_LINK_BLUETOOTH == 1

#include <BluetoothSerial.h>

extern "C" {

#include <stddef.h>

#include "zenoh-pico/link/backend/bt.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

z_result_t _z_open_bt(_z_sys_net_socket_t *sock, const char *gname, uint8_t mode, uint8_t profile, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    if (profile == _Z_BT_PROFILE_SPP) {
        sock->_bts = new BluetoothSerial();
        if (mode == _Z_BT_MODE_SLAVE) {
            sock->_bts->begin(gname, false);
        } else if (mode == _Z_BT_MODE_MASTER) {
            sock->_bts->begin(gname, true);
            uint8_t connected = sock->_bts->connect(gname);
            if (!connected) {
                while (!sock->_bts->connected(tout)) {
                    ZP_ASM_NOP;
                }
            }
        } else {
            delete sock->_bts;
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

z_result_t _z_listen_bt(_z_sys_net_socket_t *sock, const char *gname, uint8_t mode, uint8_t profile, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    if (profile == _Z_BT_PROFILE_SPP) {
        sock->_bts = new BluetoothSerial();
        if (mode == _Z_BT_MODE_SLAVE) {
            sock->_bts->begin(gname, false);
        } else if (mode == _Z_BT_MODE_MASTER) {
            sock->_bts->begin(gname, true);
            uint8_t connected = sock->_bts->connect(gname);
            if (!connected) {
                while (!sock->_bts->connected(tout)) {
                    ZP_ASM_NOP;
                }
            }
        } else {
            delete sock->_bts;
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_close_bt(_z_sys_net_socket_t *sock) {
    sock->_bts->end();
    delete sock->_bts;
}

size_t _z_read_bt(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t i = 0;
    for (i = 0; i < len; i++) {
        // flawfinder: ignore
        int c = sock._bts->read();
        if (c == -1) {
            delay(1);
            break;
        }
        ptr[i] = (uint8_t)c;
    }

    return i;
}

size_t _z_read_exact_bt(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_bt(sock, pos, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

size_t _z_send_bt(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    sock._bts->write(ptr, len);
    return len;
}

}  // extern "C"

#endif
