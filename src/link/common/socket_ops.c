//
// Copyright (c) 2026 ZettaScale Technology
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

#include "zenoh-pico/link/common/socket_ops.h"

#include <stdint.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/transport/socket.h"
#if Z_FEATURE_LINK_TLS == 1
#include "zenoh-pico/link/transport/tls_stream.h"
#endif
#include "zenoh-pico/utils/logging.h"

static size_t _z_link_peer_socket_read(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len) {
    if ((link == NULL) || (peer == NULL)) {
        return SIZE_MAX;
    }
    switch (link->_type) {
#if Z_FEATURE_LINK_TCP == 1
        case _Z_LINK_TYPE_TCP:
            return _z_tcp_read(peer->_socket, ptr, len);
#endif
#if Z_FEATURE_LINK_UDP_UNICAST == 1
        case _Z_LINK_TYPE_UDP:
            return _z_udp_unicast_read(peer->_socket, ptr, len);
#endif
#if Z_FEATURE_LINK_SERIAL == 1
        case _Z_LINK_TYPE_SERIAL:
            return _z_read_serial(peer->_socket, ptr, len);
#endif
#if Z_FEATURE_LINK_WS == 1
        case _Z_LINK_TYPE_WS:
            return _z_ws_transport_read_socket(peer->_socket, ptr, len);
#endif
#if Z_FEATURE_LINK_TLS == 1
        case _Z_LINK_TYPE_TLS:
            if (peer->_socket._tls_sock == NULL) {
                _Z_ERROR("TLS context not found in socket");
                return SIZE_MAX;
            }
            return _z_read_tls((_z_tls_socket_t *)peer->_socket._tls_sock, ptr, len);
#endif
        default:
            return SIZE_MAX;
    }
}

static size_t _z_link_peer_socket_write(const _z_link_t *link, _z_link_peer_t *peer, const uint8_t *ptr, size_t len) {
    if ((link == NULL) || (peer == NULL) || (link->_write_f == NULL)) {
        return SIZE_MAX;
    }
    return link->_write_f(link, ptr, len, &peer->_socket);
}

static void _z_link_peer_socket_iter_reset(_z_socket_wait_iter_t *iter) {
    _z_link_peer_iter_reset((_z_link_peer_iter_t *)iter->_ctx);
}

static bool _z_link_peer_socket_iter_next(_z_socket_wait_iter_t *iter) {
    return _z_link_peer_iter_next((_z_link_peer_iter_t *)iter->_ctx);
}

static const _z_sys_net_socket_t *_z_link_peer_socket_iter_get_socket(const _z_socket_wait_iter_t *iter) {
    const _z_link_peer_t *peer = _z_link_peer_iter_get_peer((const _z_link_peer_iter_t *)iter->_ctx);
    if (peer == NULL) {
        return NULL;
    }
    return _z_link_peer_get_socket_const(peer);
}

static void _z_link_peer_socket_iter_set_ready(_z_socket_wait_iter_t *iter, bool ready) {
    _z_link_peer_iter_set_ready((_z_link_peer_iter_t *)iter->_ctx, ready);
}

z_result_t _z_link_socket_open_peer(const _z_link_t *link, _z_link_peer_t *peer, const _z_string_t *locator,
                                    const _z_config_t *session_cfg) {
#if Z_FEATURE_LINK_TLS != 1
    _ZP_UNUSED(session_cfg);
#endif
    _ZP_UNUSED(link);

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
#if Z_FEATURE_LINK_TLS == 1
    } else if (_z_endpoint_tls_valid(&ep) == _Z_RES_OK) {
        ret = _z_new_peer_tls(&ep, &socket, session_cfg);
#endif
    } else {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN);
        ret = _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
    }
    _z_endpoint_clear(&ep);

    if (ret == _Z_RES_OK) {
        *peer = _z_link_peer_from_socket(socket, true);
    }
    return ret;
}

z_result_t _z_link_socket_peer_from_link(const _z_link_t *link, _z_link_peer_t *peer) {
    if (peer == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    const _z_sys_net_socket_t *socket = _z_link_get_socket(link);
    if (socket == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    *peer = _z_link_peer_from_socket(*socket, false);
    return _Z_RES_OK;
}

z_result_t _z_link_socket_wait_peers_readable(const _z_link_t *link, _z_link_peer_iter_t *peers, uint32_t timeout_ms) {
    _ZP_UNUSED(link);
    _z_socket_wait_iter_t socket_iter = {
        ._ctx = peers,
        ._current_entry = NULL,
        ._reset = _z_link_peer_socket_iter_reset,
        ._next = _z_link_peer_socket_iter_next,
        ._get_socket = _z_link_peer_socket_iter_get_socket,
        ._set_ready = _z_link_peer_socket_iter_set_ready,
    };
    return _z_socket_wait_readable(&socket_iter, timeout_ms);
}

static z_result_t _z_link_peer_socket_set_blocking(_z_link_peer_t *peer, bool blocking) {
    if (peer == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    return _z_socket_set_blocking(&peer->_socket, blocking);
}

static z_result_t _z_link_peer_socket_get_endpoints(const _z_link_peer_t *peer, char *local, size_t local_len,
                                                    char *remote, size_t remote_len) {
    if (peer == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    return _z_socket_get_endpoints(&peer->_socket, local, local_len, remote, remote_len);
}

static void _z_link_peer_socket_close(_z_link_peer_t *peer) {
    if (!peer->_owns_socket) {
        return;
    }
#if Z_FEATURE_LINK_TLS == 1
    _z_close_tls_socket(&peer->_socket);
#endif
    _z_socket_close(&peer->_socket);
    peer->_owns_socket = false;
}

static void _z_link_peer_socket_clear(_z_link_peer_t *peer) { _z_link_peer_socket_close(peer); }

static const _z_link_peer_ops_t _z_socket_peer_ops = {
    ._read_f = _z_link_peer_socket_read,
    ._write_f = _z_link_peer_socket_write,
    ._set_blocking_f = _z_link_peer_socket_set_blocking,
    ._get_endpoints_f = _z_link_peer_socket_get_endpoints,
    ._close_f = _z_link_peer_socket_close,
    ._clear_f = _z_link_peer_socket_clear,
};

const _z_link_peer_ops_t *_z_link_peer_socket_ops(void) { return &_z_socket_peer_ops; }
