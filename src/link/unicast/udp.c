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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/common/socket_ops.h"
#include "zenoh-pico/link/driver.h"
#include "zenoh-pico/link/transport/udp_unicast.h"

#if Z_FEATURE_LINK_UDP_UNICAST == 1

typedef struct {
    _z_link_t _base;
    _z_udp_socket_t _udp;
} _z_udp_unicast_link_t;

static _z_udp_unicast_link_t *_z_udp_unicast_link(_z_link_t *link) { return (_z_udp_unicast_link_t *)link; }

static const _z_udp_unicast_link_t *_z_udp_unicast_link_const(const _z_link_t *link) {
    return (const _z_udp_unicast_link_t *)link;
}

static size_t _z_link_peer_read_udp_unicast(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr,
                                            size_t len);
static size_t _z_link_peer_write_udp_unicast(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr,
                                             size_t len);

static const _z_link_peer_ops_t _z_udp_unicast_peer_ops = {
    ._read_f = _z_link_peer_read_udp_unicast,
    ._write_f = _z_link_peer_write_udp_unicast,
    ._set_blocking_f = _z_link_socket_peer_set_blocking,
    ._get_endpoints_f = _z_link_socket_peer_get_endpoints,
    ._close_f = _z_link_socket_peer_close,
};

z_result_t _z_endpoint_udp_unicast_valid(_z_endpoint_t *endpoint) {
    _z_string_t udp_str = _z_string_alias_str(UDP_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &udp_str)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return _z_udp_unicast_address_valid(&endpoint->_locator._address);
}

z_result_t _z_f_link_open_udp_unicast(_z_link_t *self) {
    _z_udp_unicast_link_t *link = _z_udp_unicast_link(self);
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    _Z_RETURN_IF_ERR(_z_udp_unicast_open(&link->_udp._sock, link->_udp._rep, tout));
    _Z_CLEAN_RETURN_IF_ERR(
        _z_link_socket_peer_from_socket(&self->_peer, link->_udp._sock, _z_udp_unicast_close, &_z_udp_unicast_peer_ops),
        _z_udp_unicast_close(&link->_udp._sock));
    return _Z_RES_OK;
}

z_result_t _z_f_link_listen_udp_unicast(_z_link_t *self) {
    _z_udp_unicast_link_t *link = _z_udp_unicast_link(self);
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    _Z_RETURN_IF_ERR(_z_udp_unicast_listen(&link->_udp._sock, link->_udp._rep, tout));
    _Z_CLEAN_RETURN_IF_ERR(
        _z_link_socket_peer_from_socket(&self->_peer, link->_udp._sock, _z_udp_unicast_close, &_z_udp_unicast_peer_ops),
        _z_udp_unicast_close(&link->_udp._sock));
    return _Z_RES_OK;
}

void _z_f_link_close_udp_unicast(_z_link_t *self) { _z_link_peer_close(&self->_peer); }

static void _z_udp_unicast_link_drop(_z_link_t *self) {
    _z_udp_unicast_link_t *link = _z_udp_unicast_link(self);
    if (link != NULL) {
        _z_udp_unicast_endpoint_clear(&link->_udp._rep);
    }
}

size_t _z_f_link_write_udp_unicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_udp_unicast_link_t *link = _z_udp_unicast_link_const(self);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return (socket == NULL) || (link == NULL) ? SIZE_MAX : _z_udp_unicast_write(*socket, ptr, len, link->_udp._rep);
}

size_t _z_f_link_write_all_udp_unicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_udp_unicast_link_t *link = _z_udp_unicast_link_const(self);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return (socket == NULL) || (link == NULL) ? SIZE_MAX : _z_udp_unicast_write(*socket, ptr, len, link->_udp._rep);
}

size_t _z_f_link_read_udp_unicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return socket == NULL ? SIZE_MAX : _z_udp_unicast_read(*socket, ptr, len);
}

size_t _z_f_link_read_exact_udp_unicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return socket == NULL ? SIZE_MAX : _z_udp_unicast_read_exact(*socket, ptr, len);
}

uint16_t _z_get_link_mtu_udp_unicast(void) {
    // @TODO: the return value should change depending on the target platform.
    return 1450;
}

static size_t _z_link_peer_read_udp_unicast(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr,
                                            size_t len) {
    _ZP_UNUSED(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    return socket == NULL ? SIZE_MAX : _z_udp_unicast_read(*socket, ptr, len);
}

static size_t _z_link_peer_write_udp_unicast(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr,
                                             size_t len) {
    const _z_udp_unicast_link_t *udp_link = _z_udp_unicast_link_const(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    if ((udp_link == NULL) || (socket == NULL)) {
        return SIZE_MAX;
    }
    return _z_udp_unicast_write(*socket, ptr, len, udp_link->_udp._rep);
}

z_result_t _z_new_link_udp_unicast(_z_link_t **zl, _z_endpoint_t *endpoint) {
    if (zl == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    *zl = NULL;

    _z_udp_unicast_link_t *link = (_z_udp_unicast_link_t *)z_malloc(sizeof(_z_udp_unicast_link_t));
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    memset(link, 0, sizeof(_z_udp_unicast_link_t));

    _z_link_t *base = &link->_base;
    base->_drop_f = _z_udp_unicast_link_drop;
    z_result_t ret = _z_udp_unicast_endpoint_init_from_address(&link->_udp._rep, &endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        z_free(link);
        return ret;
    }
    base->_endpoint = *endpoint;
    *endpoint = (_z_endpoint_t){0};

    base->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    base->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    base->_cap._is_reliable = false;

    base->_mtu = _z_get_link_mtu_udp_unicast();

    base->_close_f = _z_f_link_close_udp_unicast;

    base->_write_f = _z_f_link_write_udp_unicast;
    base->_write_all_f = _z_f_link_write_all_udp_unicast;
    base->_read_f = _z_f_link_read_udp_unicast;
    base->_read_exact_f = _z_f_link_read_exact_udp_unicast;
    base->_wait_peers_readable_f = _z_link_socket_wait_peers_readable;
    base->_peer_from_link_f = _z_link_peer_from_default;

    *zl = base;

    return _Z_RES_OK;
}

static z_result_t _z_link_driver_udp_unicast_create(_z_link_t **link, _z_endpoint_t *endpoint,
                                                    const _z_config_t *session_cfg) {
    _ZP_UNUSED(session_cfg);
    return _z_new_link_udp_unicast(link, endpoint);
}

const _z_link_driver_t _z_link_driver_udp_unicast = {
    ._validate_f = _z_endpoint_udp_unicast_valid,
    ._create_f = _z_link_driver_udp_unicast_create,
    ._open_f = _z_f_link_open_udp_unicast,
    ._listen_f = NULL,
};
#endif
