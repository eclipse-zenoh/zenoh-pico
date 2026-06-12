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

#include <stdint.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/link.h"
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
    ._close_f = _z_link_peer_socket_close,
    ._clear_f = _z_link_peer_socket_clear,
};

const _z_link_peer_ops_t *_z_link_peer_socket_ops(void) { return &_z_socket_peer_ops; }
