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

#include "zenoh-pico/link/config/tcp.h"

#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/common/socket_ops.h"
#include "zenoh-pico/link/driver.h"
#include "zenoh-pico/link/transport/tcp.h"

#if Z_FEATURE_LINK_TCP == 1

typedef struct {
    _z_tcp_socket_t _tcp;
} _z_tcp_link_state_t;

static _z_tcp_link_state_t *_z_tcp_link_state(_z_link_t *link) { return (_z_tcp_link_state_t *)_z_link_state(link); }

static size_t _z_link_peer_read_tcp(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len);
static size_t _z_link_peer_write_tcp(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr, size_t len);

static const _z_link_peer_ops_t _z_tcp_peer_ops = {
    ._read_f = _z_link_peer_read_tcp,
    ._write_f = _z_link_peer_write_tcp,
    ._set_blocking_f = _z_link_socket_peer_set_blocking,
    ._get_endpoints_f = _z_link_socket_peer_get_endpoints,
    ._close_f = _z_link_socket_peer_close,
};

z_result_t _z_endpoint_tcp_valid(_z_endpoint_t *endpoint) {
    _z_string_t tcp_str = _z_string_alias_str(TCP_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &tcp_str)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return _z_tcp_address_valid(&endpoint->_locator._address);
}

z_result_t _z_f_link_open_tcp(_z_link_t *zl) {
    _z_tcp_link_state_t *state = _z_tcp_link_state(zl);
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&zl->_endpoint._config, TCP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    _Z_RETURN_IF_ERR(_z_tcp_open(&state->_tcp._sock, state->_tcp._rep, tout));
    _Z_CLEAN_RETURN_IF_ERR(
        _z_link_socket_peer_from_socket(&zl->_peer, state->_tcp._sock, _z_tcp_close, &_z_tcp_peer_ops),
        _z_tcp_close(&state->_tcp._sock));
    return _Z_RES_OK;
}

z_result_t _z_f_link_listen_tcp(_z_link_t *zl) {
    _z_tcp_link_state_t *state = _z_tcp_link_state(zl);
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    _Z_RETURN_IF_ERR(_z_tcp_listen(&state->_tcp._sock, state->_tcp._rep));
    _Z_CLEAN_RETURN_IF_ERR(
        _z_link_socket_peer_from_socket(&zl->_peer, state->_tcp._sock, _z_tcp_close, &_z_tcp_peer_ops),
        _z_tcp_close(&state->_tcp._sock));
    return _Z_RES_OK;
}

void _z_f_link_close_tcp(_z_link_t *zl) { _z_link_peer_close(&zl->_peer); }

static void _z_tcp_link_state_drop(void *arg) {
    _z_tcp_link_state_t *state = (_z_tcp_link_state_t *)arg;
    if (state != NULL) {
        _z_tcp_endpoint_clear(&state->_tcp._rep);
        z_free(state);
    }
}

size_t _z_f_link_write_tcp(const _z_link_t *zl, const uint8_t *ptr, size_t len) {
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&zl->_peer);
    return socket == NULL ? SIZE_MAX : _z_tcp_write(*socket, ptr, len);
}

size_t _z_f_link_write_all_tcp(const _z_link_t *zl, const uint8_t *ptr, size_t len) {
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&zl->_peer);
    return socket == NULL ? SIZE_MAX : _z_tcp_write(*socket, ptr, len);
}

size_t _z_f_link_read_tcp(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&zl->_peer);
    return socket == NULL ? SIZE_MAX : _z_tcp_read(*socket, ptr, len);
}

size_t _z_f_link_read_exact_tcp(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&zl->_peer);
    return socket == NULL ? SIZE_MAX : _z_tcp_read_exact(*socket, ptr, len);
}

uint16_t _z_get_link_mtu_tcp(void) {
    // Maximum MTU for TCP
    return 65535;
}

z_result_t _z_new_peer_tcp(_z_endpoint_t *endpoint, _z_sys_net_socket_t *socket) {
    _z_sys_net_endpoint_t sys_endpoint = {0};
    _Z_CLEAN_RETURN_IF_ERR(_z_tcp_endpoint_init_from_address(&sys_endpoint, &endpoint->_locator._address),
                           _z_tcp_endpoint_clear(&sys_endpoint));

    z_result_t ret = _z_tcp_open(socket, sys_endpoint, Z_CONFIG_SOCKET_TIMEOUT);
    _z_tcp_endpoint_clear(&sys_endpoint);
    return ret;
}

static size_t _z_link_peer_read_tcp(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len) {
    _ZP_UNUSED(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    return socket == NULL ? SIZE_MAX : _z_tcp_read(*socket, ptr, len);
}

static size_t _z_link_peer_write_tcp(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr,
                                     size_t len) {
    _ZP_UNUSED(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    return socket == NULL ? SIZE_MAX : _z_tcp_write(*socket, ptr, len);
}

static z_result_t _z_f_link_open_peer_tcp(const _z_link_t *link, _z_link_peer_t *peer, const _z_string_t *locator,
                                          const _z_config_t *session_cfg) {
    _ZP_UNUSED(link);
    _ZP_UNUSED(session_cfg);

    _z_endpoint_t ep;
    z_result_t ret = _z_endpoint_from_string(&ep, locator);
    if (ret != _Z_RES_OK) {
        _z_endpoint_clear(&ep);
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    _z_sys_net_socket_t socket = {0};
    if (_z_endpoint_tcp_valid(&ep) == _Z_RES_OK) {
        ret = _z_new_peer_tcp(&ep, &socket);
    } else {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN);
        ret = _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
    }
    _z_endpoint_clear(&ep);

    if (ret != _Z_RES_OK) {
        return ret;
    }
    _Z_CLEAN_RETURN_IF_ERR(_z_link_socket_peer_from_socket(peer, socket, _z_tcp_close, &_z_tcp_peer_ops),
                           _z_tcp_close(&socket));
    return _Z_RES_OK;
}

static z_result_t _z_f_link_accept_tcp(const _z_link_t *link, _z_link_peer_t *peer) {
    if ((link == NULL) || (peer == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    const _z_sys_net_socket_t *listen_socket = _z_link_socket_peer_get_socket_const(&link->_peer);
    if (listen_socket == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    _z_sys_net_socket_t con_socket = {0};
    _Z_RETURN_IF_ERR(_z_tcp_accept(listen_socket, &con_socket));
    _Z_CLEAN_RETURN_IF_ERR(_z_link_socket_peer_from_socket(peer, con_socket, _z_tcp_close, &_z_tcp_peer_ops),
                           _z_tcp_close(&con_socket));

    _Z_CLEAN_RETURN_IF_ERR(_z_link_socket_peer_set_blocking(peer, true), _z_link_peer_clear(peer));
    return _Z_RES_OK;
}

z_result_t _z_new_link_tcp(_z_link_t *zl, _z_endpoint_t *endpoint) {
    _z_tcp_link_state_t *state = (_z_tcp_link_state_t *)z_malloc(sizeof(_z_tcp_link_state_t));
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    memset(state, 0, sizeof(_z_tcp_link_state_t));

    _Z_CLEAN_RETURN_IF_ERR(_z_tcp_endpoint_init_from_address(&state->_tcp._rep, &endpoint->_locator._address),
                           z_free(state));

    zl->_state = state;
    zl->_state_drop_f = _z_tcp_link_state_drop;
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    zl->_cap._flow = Z_LINK_CAP_FLOW_STREAM;
    zl->_cap._is_reliable = true;

    zl->_mtu = _z_get_link_mtu_tcp();

    zl->_endpoint = *endpoint;

    zl->_close_f = _z_f_link_close_tcp;

    zl->_write_f = _z_f_link_write_tcp;
    zl->_write_all_f = _z_f_link_write_all_tcp;
    zl->_read_f = _z_f_link_read_tcp;
    zl->_read_exact_f = _z_f_link_read_exact_tcp;
    zl->_wait_peers_readable_f = _z_link_socket_wait_peers_readable;
    zl->_open_peer_f = _z_f_link_open_peer_tcp;
    zl->_peer_from_link_f = _z_link_peer_from_default;
    zl->_accept_peer_f = _z_f_link_accept_tcp;

    return _Z_RES_OK;
}

static z_result_t _z_link_driver_tcp_create(_z_link_t *link, _z_endpoint_t *endpoint, const _z_config_t *session_cfg) {
    _ZP_UNUSED(session_cfg);
    return _z_new_link_tcp(link, endpoint);
}

const _z_link_driver_t _z_link_driver_tcp = {
    ._validate_f = _z_endpoint_tcp_valid,
    ._create_f = _z_link_driver_tcp_create,
    ._open_f = _z_f_link_open_tcp,
    ._listen_f = _z_f_link_listen_tcp,
};
#else
z_result_t _z_endpoint_tcp_valid(_z_endpoint_t *endpoint) {
    _ZP_UNUSED(endpoint);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}

z_result_t _z_new_peer_tcp(_z_endpoint_t *endpoint, _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(endpoint);
    _ZP_UNUSED(socket);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}

z_result_t _z_new_link_tcp(_z_link_t *zl, _z_endpoint_t *endpoint) {
    _ZP_UNUSED(zl);
    _ZP_UNUSED(endpoint);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}
#endif
