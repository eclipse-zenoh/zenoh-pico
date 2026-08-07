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
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/driver.h"
#include "zenoh-pico/link/transport/bt.h"
#include "zenoh-pico/utils/string.h"

#if Z_FEATURE_LINK_BLUETOOTH == 1

#define SPP_MAXIMUM_PAYLOAD 128

typedef struct {
    _z_link_t _base;
    _z_bt_socket_t _bt;
    size_t _gname_len;
} _z_bt_link_t;

static _z_bt_link_t *_z_bt_link(_z_link_t *link) { return (_z_bt_link_t *)link; }

static const _z_bt_link_t *_z_bt_link_const(const _z_link_t *link) { return (const _z_bt_link_t *)link; }

z_result_t _z_endpoint_bt_valid(_z_endpoint_t *ep) {
    _z_string_t bt_str = _z_string_alias_str(BT_SCHEMA);
    if (!_z_string_equals(&ep->_locator._protocol, &bt_str)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    if (_z_string_len(&ep->_locator._address) == (size_t)0) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return _Z_RES_OK;
}

static char *__z_convert_address_bt(_z_string_t *address, size_t *len) {
    *len = _z_string_len(address);
    char *ret = (char *)z_malloc(*len + 1);
    if (ret != NULL) {
        _z_str_n_copy(ret, _z_string_data(address), *len + 1);
    }
    return ret;
}

static bool _z_bt_copy_remote_addr(const _z_bt_link_t *link, _z_slice_t *addr) {
    if (addr == NULL) {
        return true;
    }
    if (link->_bt._gname == NULL) {
        return false;
    }
    size_t offset = 0;
    if (!_z_memcpy_checked((uint8_t *)addr->start, addr->len, &offset, link->_bt._gname, link->_gname_len)) {
        return false;
    }
    addr->len = link->_gname_len;
    return true;
}

z_result_t _z_f_link_open_bt(_z_link_t *self) {
    _z_bt_link_t *link = _z_bt_link(self);
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    const char *mode_str = _z_str_intmap_get(&self->_endpoint._config, BT_CONFIG_MODE_KEY);
    uint8_t mode = (strcmp(mode_str, "master") == 0) ? _Z_BT_MODE_MASTER : _Z_BT_MODE_SLAVE;
    const char *profile_str = _z_str_intmap_get(&self->_endpoint._config, BT_CONFIG_PROFILE_KEY);
    uint8_t profile = (strcmp(profile_str, "spp") == 0) ? _Z_BT_PROFILE_SPP : _Z_BT_PROFILE_UNSUPPORTED;
    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, BT_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    link->_bt._gname = __z_convert_address_bt(&self->_endpoint._locator._address, &link->_gname_len);
    return _z_open_bt(&link->_bt._sock, link->_bt._gname, mode, profile, tout);
}

z_result_t _z_f_link_listen_bt(_z_link_t *self) {
    _z_bt_link_t *link = _z_bt_link(self);
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    const char *mode_str = _z_str_intmap_get(&self->_endpoint._config, BT_CONFIG_MODE_KEY);
    uint8_t mode = (strcmp(mode_str, "master") == 0) ? _Z_BT_MODE_MASTER : _Z_BT_MODE_SLAVE;
    const char *profile_str = _z_str_intmap_get(&self->_endpoint._config, BT_CONFIG_PROFILE_KEY);
    uint8_t profile = (strcmp(profile_str, "spp") == 0) ? _Z_BT_PROFILE_SPP : _Z_BT_PROFILE_UNSUPPORTED;
    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, BT_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    link->_bt._gname = __z_convert_address_bt(&self->_endpoint._locator._address, &link->_gname_len);
    return _z_listen_bt(&link->_bt._sock, link->_bt._gname, mode, profile, tout);
}

void _z_f_link_close_bt(_z_link_t *self) {
    _z_bt_link_t *link = _z_bt_link(self);
    if (link != NULL) {
        _z_close_bt(&link->_bt._sock);
    }
}

static void _z_bt_link_drop(_z_link_t *self) {
    _z_bt_link_t *link = _z_bt_link(self);
    if (link != NULL) {
        z_free(link->_bt._gname);
    }
}

size_t _z_f_link_write_bt(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_bt_link_t *link = _z_bt_link_const(self);
    return link == NULL ? SIZE_MAX : _z_send_bt(link->_bt._sock, ptr, len);
}

size_t _z_f_link_write_all_bt(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_bt_link_t *link = _z_bt_link_const(self);
    return link == NULL ? SIZE_MAX : _z_send_bt(link->_bt._sock, ptr, len);
}

size_t _z_f_link_read_bt(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    const _z_bt_link_t *link = _z_bt_link_const(self);
    if (link == NULL) {
        return SIZE_MAX;
    }
    size_t rb = _z_read_bt(link->_bt._sock, ptr, len);
    if ((rb > (size_t)0) && !_z_bt_copy_remote_addr(link, addr)) {
        return SIZE_MAX;
    }

    return rb;
}

size_t _z_f_link_read_exact_bt(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    const _z_bt_link_t *link = _z_bt_link_const(self);
    if (link == NULL) {
        return SIZE_MAX;
    }
    size_t rb = _z_read_exact_bt(link->_bt._sock, ptr, len);
    if ((rb == len) && !_z_bt_copy_remote_addr(link, addr)) {
        return SIZE_MAX;
    }

    return rb;
}

uint16_t _z_get_link_mtu_bt(void) { return SPP_MAXIMUM_PAYLOAD; }

z_result_t _z_new_link_bt(_z_link_t **zl, _z_endpoint_t *endpoint) {
    if (zl == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    *zl = NULL;

    _z_bt_link_t *link = (_z_bt_link_t *)z_malloc(sizeof(_z_bt_link_t));
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    memset(link, 0, sizeof(_z_bt_link_t));

    _z_link_t *base = &link->_base;
    base->_endpoint = *endpoint;
    *endpoint = (_z_endpoint_t){0};
    base->_drop_f = _z_bt_link_drop;
    base->_cap._transport = Z_LINK_CAP_TRANSPORT_MULTICAST;
    base->_cap._flow = Z_LINK_CAP_FLOW_STREAM;
    base->_cap._is_reliable = false;

    base->_mtu = _z_get_link_mtu_bt();

    base->_close_f = _z_f_link_close_bt;

    base->_write_f = _z_f_link_write_bt;
    base->_write_all_f = _z_f_link_write_all_bt;
    base->_read_f = _z_f_link_read_bt;
    base->_read_exact_f = _z_f_link_read_exact_bt;

    *zl = base;

    return _Z_RES_OK;
}

static z_result_t _z_link_driver_bt_create(_z_link_t **link, _z_endpoint_t *endpoint, const _z_config_t *session_cfg) {
    _ZP_UNUSED(session_cfg);
    return _z_new_link_bt(link, endpoint);
}

const _z_link_driver_t _z_link_driver_bt = {
    ._validate_f = _z_endpoint_bt_valid,
    ._create_f = _z_link_driver_bt_create,
    ._open_f = _z_f_link_open_bt,
    ._listen_f = _z_f_link_listen_bt,
};
#endif
