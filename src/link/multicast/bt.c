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

#include "zenoh-pico/link/config/bt.h"

#include <stddef.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/system/link/bt.h"

#if Z_LINK_BLUETOOTH == 1

#define SPP_MAXIMUM_PAYLOAD 128

int8_t _z_f_link_open_bt(_z_link_t *self) {
    int8_t ret = _Z_RES_OK;

    const char *mode_str = _z_str_intmap_get(&self->_endpoint._config, BT_CONFIG_MODE_KEY);
    uint8_t mode = (strcmp(mode_str, "master") == 0) ? _Z_BT_MODE_MASTER : _Z_BT_MODE_SLAVE;
    const char *profile_str = _z_str_intmap_get(&self->_endpoint._config, BT_CONFIG_PROFILE_KEY);
    uint8_t profile = (strcmp(profile_str, "spp") == 0) ? _Z_BT_PROFILE_SPP : _Z_BT_PROFILE_UNSUPPORTED;

    self->_socket._bt._gname = self->_endpoint._locator._address;
    self->_socket._bt._sock = _z_open_bt(self->_endpoint._locator._address, mode, profile);
    if (self->_socket._bt._sock._err == true) {
        ret = -1;
    }

    return ret;
}

int8_t _z_f_link_listen_bt(_z_link_t *self) {
    int8_t ret = _Z_RES_OK;

    uint8_t mode = 0;
    const char *mode_str = _z_str_intmap_get(&self->_endpoint._config, BT_CONFIG_MODE_KEY);
    if (strcmp(mode_str, "master") == 0) {
        mode = _Z_BT_MODE_MASTER;
    } else if (strcmp(mode_str, "slave") == 0) {
        mode = _Z_BT_MODE_SLAVE;
    } else {
        ret = -1;
    }

    uint8_t profile = 0;
    const char *profile_str = _z_str_intmap_get(&self->_endpoint._config, BT_CONFIG_PROFILE_KEY);
    if (strcmp(profile_str, "spp") == 0) {
        profile = _Z_BT_PROFILE_SPP;
    } else {
        ret = -1;
    }

    self->_socket._bt._gname = self->_endpoint._locator._address;
    self->_socket._bt._sock = _z_listen_bt(self->_endpoint._locator._address, mode, profile);
    if (self->_socket._bt._sock._err == true) {
        ret = -1;
    }

    return ret;
}

void _z_f_link_close_bt(_z_link_t *self) { _z_close_bt(self->_socket._bt._sock); }

void _z_f_link_free_bt(_z_link_t *self) { _z_str_free(&self->_socket._bt._gname); }

size_t _z_f_link_write_bt(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    return _z_send_bt(self->_socket._bt._sock, ptr, len);
}

size_t _z_f_link_write_all_bt(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    return _z_send_bt(self->_socket._bt._sock, ptr, len);
}

size_t _z_f_link_read_bt(const _z_link_t *self, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    size_t rb = _z_read_bt(self->_socket._bt._sock, ptr, len);
    if ((rb > (size_t)0) && (addr != NULL)) {
        *addr = _z_bytes_make(strlen(self->_socket._bt._gname));
        (void)memcpy((uint8_t *)addr->start, self->_socket._bt._gname, addr->len);
    }

    return rb;
}

size_t _z_f_link_read_exact_bt(const _z_link_t *self, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    size_t rb = _z_read_exact_bt(self->_socket._bt._sock, ptr, len);
    if ((rb == len) && (addr != NULL)) {
        *addr = _z_bytes_make(strlen(self->_socket._bt._gname));
        (void)memcpy((uint8_t *)addr->start, self->_socket._bt._gname, addr->len);
    }

    return rb;
}

uint16_t _z_get_link_mtu_bt(void) { return SPP_MAXIMUM_PAYLOAD; }

_z_link_t *_z_new_link_bt(_z_endpoint_t endpoint) {
    _z_link_t *lt = (_z_link_t *)z_malloc(sizeof(_z_link_t));

    if (lt != NULL) {
        lt->_capabilities = Z_LINK_CAPABILITY_STREAMED | Z_LINK_CAPABILITY_MULTICAST;
        lt->_mtu = _z_get_link_mtu_bt();

        lt->_endpoint = endpoint;
        lt->_socket._bt._sock._err = true;

        lt->_open_f = _z_f_link_open_bt;
        lt->_listen_f = _z_f_link_listen_bt;
        lt->_close_f = _z_f_link_close_bt;
        lt->_free_f = _z_f_link_free_bt;

        lt->_write_f = _z_f_link_write_bt;
        lt->_write_all_f = _z_f_link_write_all_bt;
        lt->_read_f = _z_f_link_read_bt;
        lt->_read_exact_f = _z_f_link_read_exact_bt;
    }

    return lt;
}
#endif
