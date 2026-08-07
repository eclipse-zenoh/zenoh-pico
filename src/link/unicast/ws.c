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

#include "zenoh-pico/link/config/ws.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/common/socket_ops.h"
#include "zenoh-pico/link/driver.h"
#include "zenoh-pico/link/transport/tcp.h"
#include "zenoh-pico/link/transport/ws.h"

#if Z_FEATURE_LINK_WS == 1

typedef struct {
    _z_link_t _base;
    _z_ws_socket_t _ws;
} _z_ws_link_t;

static _z_ws_link_t *_z_ws_link(_z_link_t *link) { return (_z_ws_link_t *)link; }

static const _z_ws_link_t *_z_ws_link_const(const _z_link_t *link) { return (const _z_ws_link_t *)link; }

static size_t _z_link_peer_read_ws(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len);
static size_t _z_link_peer_write_ws(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr, size_t len);

static const _z_link_peer_ops_t _z_ws_peer_ops = {
    ._read_f = _z_link_peer_read_ws,
    ._write_f = _z_link_peer_write_ws,
    ._set_blocking_f = _z_link_socket_peer_set_blocking,
    ._get_endpoints_f = _z_link_socket_peer_get_endpoints,
    ._close_f = _z_link_socket_peer_close,
};

static z_result_t _z_ws_address_valid(const _z_string_t *address) { return _z_tcp_address_valid(address); }

z_result_t _z_endpoint_ws_valid(_z_endpoint_t *endpoint) {
    _z_string_t str = _z_string_alias_str(WS_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &str)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return _z_ws_address_valid(&endpoint->_locator._address);
}

z_result_t _z_f_link_open_ws(_z_link_t *zl) {
    _z_ws_link_t *link = _z_ws_link(zl);
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&zl->_endpoint._config, WS_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    _Z_RETURN_IF_ERR(_z_ws_transport_open(&link->_ws, tout));
    // WebSocket peer handles borrow the link-owned transport; only link teardown closes it.
    _Z_CLEAN_RETURN_IF_ERR(_z_link_socket_peer_from_socket(&zl->_peer, link->_ws._sock, NULL, &_z_ws_peer_ops),
                           _z_ws_transport_close(&link->_ws));
    return _Z_RES_OK;
}

z_result_t _z_f_link_listen_ws(_z_link_t *zl) {
    _z_ws_link_t *link = _z_ws_link(zl);
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    _Z_RETURN_IF_ERR(_z_ws_transport_listen(&link->_ws));
    // WebSocket peer handles borrow the link-owned transport; only link teardown closes it.
    _Z_CLEAN_RETURN_IF_ERR(_z_link_socket_peer_from_socket(&zl->_peer, link->_ws._sock, NULL, &_z_ws_peer_ops),
                           _z_ws_transport_close(&link->_ws));
    return _Z_RES_OK;
}

void _z_f_link_close_ws(_z_link_t *zl) {
    _z_ws_link_t *link = _z_ws_link(zl);
    if (link != NULL) {
        _z_ws_transport_close(&link->_ws);
    }
}

static void _z_ws_link_drop(_z_link_t *zl) {
    _z_ws_link_t *link = _z_ws_link(zl);
    if (link != NULL) {
        _z_ws_endpoint_clear(&link->_ws._rep);
    }
}

size_t _z_f_link_write_ws(const _z_link_t *zl, const uint8_t *ptr, size_t len) {
    const _z_ws_link_t *link = _z_ws_link_const(zl);
    return link == NULL ? SIZE_MAX : _z_ws_transport_write(&link->_ws, ptr, len);
}

size_t _z_f_link_write_all_ws(const _z_link_t *zl, const uint8_t *ptr, size_t len) {
    const _z_ws_link_t *link = _z_ws_link_const(zl);
    return link == NULL ? SIZE_MAX : _z_ws_transport_write(&link->_ws, ptr, len);
}

size_t _z_f_link_read_ws(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_ws_link_t *link = _z_ws_link_const(zl);
    return link == NULL ? SIZE_MAX : _z_ws_transport_read(&link->_ws, ptr, len);
}

size_t _z_f_link_read_exact_ws(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_ws_link_t *link = _z_ws_link_const(zl);
    return link == NULL ? SIZE_MAX : _z_ws_transport_read_exact(&link->_ws, ptr, len);
}

uint16_t _z_get_link_mtu_ws(void) {
    // Maximum MTU for TCP
    return 65535;
}

static size_t _z_link_peer_read_ws(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len) {
    _ZP_UNUSED(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    return socket == NULL ? SIZE_MAX : _z_ws_transport_read_socket(*socket, ptr, len);
}

static size_t _z_link_peer_write_ws(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr, size_t len) {
    _ZP_UNUSED(peer);
    const _z_ws_link_t *ws_link = _z_ws_link_const(link);
    return ws_link == NULL ? SIZE_MAX : _z_ws_transport_write(&ws_link->_ws, ptr, len);
}

z_result_t _z_new_link_ws(_z_link_t **zl, _z_endpoint_t *endpoint) {
    if (zl == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    *zl = NULL;

    _z_ws_link_t *link = (_z_ws_link_t *)z_malloc(sizeof(_z_ws_link_t));
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    memset(link, 0, sizeof(_z_ws_link_t));

    _z_link_t *base = &link->_base;
    base->_drop_f = _z_ws_link_drop;
    z_result_t ret = _z_ws_endpoint_init(&link->_ws._rep, &endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        z_free(link);
        return ret;
    }
    base->_endpoint = *endpoint;
    *endpoint = (_z_endpoint_t){0};

    base->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    base->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    base->_cap._is_reliable = true;

    base->_mtu = _z_get_link_mtu_ws();

    base->_close_f = _z_f_link_close_ws;

    base->_write_f = _z_f_link_write_ws;
    base->_write_all_f = _z_f_link_write_all_ws;
    base->_read_f = _z_f_link_read_ws;
    base->_read_exact_f = _z_f_link_read_exact_ws;
    base->_wait_peers_readable_f = _z_link_socket_wait_peers_readable;
    base->_peer_from_link_f = _z_link_peer_from_default;

    *zl = base;

    return _Z_RES_OK;
}

static z_result_t _z_link_driver_ws_create(_z_link_t **link, _z_endpoint_t *endpoint, const _z_config_t *session_cfg) {
    _ZP_UNUSED(session_cfg);
    return _z_new_link_ws(link, endpoint);
}

const _z_link_driver_t _z_link_driver_ws = {
    ._validate_f = _z_endpoint_ws_valid,
    ._create_f = _z_link_driver_ws_create,
    ._open_f = _z_f_link_open_ws,
    ._listen_f = NULL,
};
#endif
