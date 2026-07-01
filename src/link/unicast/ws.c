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
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/transport/tcp.h"
#include "zenoh-pico/link/transport/ws.h"

#if Z_FEATURE_LINK_WS == 1

typedef struct {
    _z_ws_socket_t _ws;
} _z_ws_link_state_t;

static _z_ws_link_state_t *_z_ws_link_state(_z_link_t *link) { return (_z_ws_link_state_t *)_z_link_state(link); }

static const _z_ws_link_state_t *_z_ws_link_state_const(const _z_link_t *link) {
    return (const _z_ws_link_state_t *)_z_link_state_const(link);
}

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

bool _z_endpoint_ws_matches(const _z_endpoint_t *endpoint) {
    _z_string_t str = _z_string_alias_str(WS_SCHEMA);
    return _z_string_equals(&endpoint->_locator._protocol, &str);
}

z_result_t _z_endpoint_ws_valid(_z_endpoint_t *endpoint) {
    if (!_z_endpoint_ws_matches(endpoint)) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _z_ws_address_valid(&endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
    }
    return ret;
}

z_result_t _z_f_link_open_ws(_z_link_t *zl) {
    _z_ws_link_state_t *state = _z_ws_link_state(zl);
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&zl->_endpoint._config, WS_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    z_result_t ret = _z_ws_transport_open(&state->_ws, tout);
    if (ret == _Z_RES_OK) {
        // WebSocket peer handles borrow the link-owned transport; only link teardown closes it.
        ret = _z_link_socket_peer_from_socket(&zl->_peer, state->_ws._sock, NULL, &_z_ws_peer_ops);
        if (ret != _Z_RES_OK) {
            _z_ws_transport_close(&state->_ws);
        }
    }
    return ret;
}

z_result_t _z_f_link_listen_ws(_z_link_t *zl) {
    _z_ws_link_state_t *state = _z_ws_link_state(zl);
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    z_result_t ret = _z_ws_transport_listen(&state->_ws);
    if (ret == _Z_RES_OK) {
        // WebSocket peer handles borrow the link-owned transport; only link teardown closes it.
        ret = _z_link_socket_peer_from_socket(&zl->_peer, state->_ws._sock, NULL, &_z_ws_peer_ops);
        if (ret != _Z_RES_OK) {
            _z_ws_transport_close(&state->_ws);
        }
    }
    return ret;
}

void _z_f_link_close_ws(_z_link_t *zl) {
    _z_ws_link_state_t *state = _z_ws_link_state(zl);
    if (state != NULL) {
        _z_ws_transport_close(&state->_ws);
    }
}

void _z_f_link_free_ws(_z_link_t *zl) {
    _z_ws_link_state_t *state = _z_ws_link_state(zl);
    if (state != NULL) {
        _z_ws_endpoint_clear(&state->_ws._rep);
    }
}

size_t _z_f_link_write_ws(const _z_link_t *zl, const uint8_t *ptr, size_t len) {
    const _z_ws_link_state_t *state = _z_ws_link_state_const(zl);
    return state == NULL ? SIZE_MAX : _z_ws_transport_write(&state->_ws, ptr, len);
}

size_t _z_f_link_write_all_ws(const _z_link_t *zl, const uint8_t *ptr, size_t len) {
    const _z_ws_link_state_t *state = _z_ws_link_state_const(zl);
    return state == NULL ? SIZE_MAX : _z_ws_transport_write(&state->_ws, ptr, len);
}

size_t _z_f_link_read_ws(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_ws_link_state_t *state = _z_ws_link_state_const(zl);
    return state == NULL ? SIZE_MAX : _z_ws_transport_read(&state->_ws, ptr, len);
}

size_t _z_f_link_read_exact_ws(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_ws_link_state_t *state = _z_ws_link_state_const(zl);
    return state == NULL ? SIZE_MAX : _z_ws_transport_read_exact(&state->_ws, ptr, len);
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
    const _z_ws_link_state_t *state = _z_ws_link_state_const(link);
    return state == NULL ? SIZE_MAX : _z_ws_transport_write(&state->_ws, ptr, len);
}

static z_result_t _z_f_link_peer_from_link_ws(const _z_link_t *link, _z_link_peer_t *peer) {
    if ((link == NULL) || (peer == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    *peer = _z_link_peer_clone(&link->_peer);
    return _z_link_peer_check(peer) ? _Z_RES_OK : _Z_ERR_GENERIC;
}

z_result_t _z_new_link_ws(_z_link_t *zl, _z_endpoint_t *endpoint) {
    _Z_RETURN_IF_ERR(_z_endpoint_ws_valid(endpoint));

    _z_ws_link_state_t *state = (_z_ws_link_state_t *)z_malloc(sizeof(_z_ws_link_state_t));
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    memset(state, 0, sizeof(_z_ws_link_state_t));

    z_result_t ret = _z_ws_endpoint_init(&state->_ws._rep, &endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        z_free(state);
        return ret;
    }

    zl->_state = state;
    zl->_drop_f = z_free;
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    zl->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    zl->_cap._is_reliable = true;

    zl->_mtu = _z_get_link_mtu_ws();

    zl->_endpoint = *endpoint;

    zl->_open_f = _z_f_link_open_ws;
    zl->_listen_f = _z_f_link_listen_ws;
    zl->_close_f = _z_f_link_close_ws;
    zl->_free_f = _z_f_link_free_ws;

    zl->_write_f = _z_f_link_write_ws;
    zl->_write_all_f = _z_f_link_write_all_ws;
    zl->_read_f = _z_f_link_read_ws;
    zl->_read_exact_f = _z_f_link_read_exact_ws;
    zl->_wait_peers_readable_f = _z_link_socket_wait_peers_readable;
    zl->_peer_from_link_f = _z_f_link_peer_from_link_ws;

    return ret;
}
#endif
