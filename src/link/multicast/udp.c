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

#include "zenoh-pico/link/config/udp.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/driver.h"
#include "zenoh-pico/link/transport/udp_multicast.h"
#include "zenoh-pico/link/transport/udp_unicast.h"

#if Z_FEATURE_LINK_UDP_MULTICAST == 1

typedef struct {
    _z_link_t _base;
    _z_udp_socket_t _udp;
} _z_udp_multicast_link_t;

static _z_udp_multicast_link_t *_z_udp_multicast_link(_z_link_t *link) { return (_z_udp_multicast_link_t *)link; }

static const _z_udp_multicast_link_t *_z_udp_multicast_link_const(const _z_link_t *link) {
    return (const _z_udp_multicast_link_t *)link;
}

z_result_t _z_endpoint_udp_multicast_valid(_z_endpoint_t *endpoint) {
    _z_string_t udp_str = _z_string_alias_str(UDP_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &udp_str)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _z_udp_unicast_address_valid(&endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        return ret;
    }

    const char *iface = _z_str_intmap_get(&endpoint->_config, UDP_CONFIG_IFACE_KEY);
    if (iface == NULL) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return _Z_RES_OK;
}

z_result_t _z_f_link_listen_udp_multicast(_z_link_t *self) {
    _z_udp_multicast_link_t *link = _z_udp_multicast_link(self);
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    z_result_t ret = _Z_RES_OK;

    const char *iface = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_IFACE_KEY);
    const char *join = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_JOIN_KEY);
    ret = _z_udp_multicast_listen(&link->_udp._sock, link->_udp._rep, Z_CONFIG_SOCKET_TIMEOUT, iface, join);
    ret |= _z_udp_multicast_open(&link->_udp._msock, link->_udp._rep, &link->_udp._lep, Z_CONFIG_SOCKET_TIMEOUT, iface);

    return ret;
}

void _z_f_link_close_udp_multicast(_z_link_t *self) {
    _z_udp_multicast_link_t *link = _z_udp_multicast_link(self);
    if (link != NULL) {
        _z_udp_multicast_close(&link->_udp._sock, &link->_udp._msock, link->_udp._rep, link->_udp._lep);
    }
}

static void _z_udp_multicast_link_drop(_z_link_t *self) {
    _z_udp_multicast_link_t *link = _z_udp_multicast_link(self);
    if (link != NULL) {
        _z_udp_multicast_endpoint_clear(&link->_udp._lep);
        _z_udp_multicast_endpoint_clear(&link->_udp._rep);
    }
}

size_t _z_f_link_write_udp_multicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_udp_multicast_link_t *link = _z_udp_multicast_link_const(self);
    return link == NULL ? SIZE_MAX : _z_udp_multicast_write(link->_udp._msock, ptr, len, link->_udp._rep);
}

size_t _z_f_link_write_all_udp_multicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_udp_multicast_link_t *link = _z_udp_multicast_link_const(self);
    return link == NULL ? SIZE_MAX : _z_udp_multicast_write(link->_udp._msock, ptr, len, link->_udp._rep);
}

size_t _z_f_link_read_udp_multicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    const _z_udp_multicast_link_t *link = _z_udp_multicast_link_const(self);
    return link == NULL ? SIZE_MAX : _z_udp_multicast_read(link->_udp._sock, ptr, len, link->_udp._lep, addr);
}

size_t _z_f_link_read_exact_udp_multicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    const _z_udp_multicast_link_t *link = _z_udp_multicast_link_const(self);
    return link == NULL ? SIZE_MAX : _z_udp_multicast_read_exact(link->_udp._sock, ptr, len, link->_udp._lep, addr);
}

uint16_t _z_get_link_mtu_udp_multicast(void) {
    // @TODO: the return value should change depending on the target platform.
    return 1450;
}

z_result_t _z_new_link_udp_multicast(_z_link_t **zl, _z_endpoint_t *endpoint) {
    if (zl == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    *zl = NULL;

    _z_udp_multicast_link_t *link = (_z_udp_multicast_link_t *)z_malloc(sizeof(_z_udp_multicast_link_t));
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    memset(link, 0, sizeof(_z_udp_multicast_link_t));

    _z_link_t *base = &link->_base;
    base->_drop_f = _z_udp_multicast_link_drop;
    z_result_t ret = _z_udp_multicast_endpoint_init_from_address(&link->_udp._rep, &endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        z_free(link);
        return ret;
    }
    base->_endpoint = *endpoint;
    *endpoint = (_z_endpoint_t){0};
    memset(&link->_udp._lep, 0, sizeof(link->_udp._lep));

    base->_cap._transport = Z_LINK_CAP_TRANSPORT_MULTICAST;
    base->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    base->_cap._is_reliable = false;

    base->_mtu = _z_get_link_mtu_udp_multicast();

    base->_close_f = _z_f_link_close_udp_multicast;

    base->_write_f = _z_f_link_write_udp_multicast;
    base->_write_all_f = _z_f_link_write_all_udp_multicast;
    base->_read_f = _z_f_link_read_udp_multicast;
    base->_read_exact_f = _z_f_link_read_exact_udp_multicast;

    *zl = base;

    return _Z_RES_OK;
}

static z_result_t _z_link_driver_udp_multicast_create(_z_link_t **link, _z_endpoint_t *endpoint,
                                                      const _z_config_t *session_cfg) {
    _ZP_UNUSED(session_cfg);
    return _z_new_link_udp_multicast(link, endpoint);
}

const _z_link_driver_t _z_link_driver_udp_multicast = {
    ._validate_f = _z_endpoint_udp_multicast_valid,
    ._create_f = _z_link_driver_udp_multicast_create,
    ._open_f = NULL,
    ._listen_f = _z_f_link_listen_udp_multicast,
};

#endif
