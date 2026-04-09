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

#include "zenoh-pico/link/backend/tcp.h"
#include "zenoh-pico/link/backend/ws.h"

#if Z_FEATURE_LINK_WS == 1

z_result_t _z_ws_endpoint_init(_z_sys_net_endpoint_t *ep, const _z_string_t *address) {
    return _z_tcp_endpoint_init_from_address(ep, address);
}

void _z_ws_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_tcp_endpoint_clear(ep); }

z_result_t _z_ws_transport_open(_z_ws_socket_t *sock, uint32_t tout) {
    return _z_tcp_open(&sock->_sock, sock->_rep, tout);
}

z_result_t _z_ws_transport_listen(_z_ws_socket_t *sock) { return _z_tcp_listen(&sock->_sock, sock->_rep); }

void _z_ws_transport_close(_z_ws_socket_t *sock) { _z_tcp_close(&sock->_sock); }

size_t _z_ws_transport_read(const _z_ws_socket_t *sock, uint8_t *ptr, size_t len) {
    return _z_tcp_read(sock->_sock, ptr, len);
}

size_t _z_ws_transport_read_exact(const _z_ws_socket_t *sock, uint8_t *ptr, size_t len) {
    return _z_tcp_read_exact(sock->_sock, ptr, len);
}

size_t _z_ws_transport_write(const _z_ws_socket_t *sock, const uint8_t *ptr, size_t len) {
    return _z_tcp_write(sock->_sock, ptr, len);
}

size_t _z_ws_transport_read_socket(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len) {
    return _z_tcp_read(socket, ptr, len);
}

#endif
