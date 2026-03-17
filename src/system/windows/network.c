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

#include <winsock2.h>
// The following includes must come after winsock2
#include <iphlpapi.h>
#include <stdio.h>
#include <ws2tcpip.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/backend/socket.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/mutex.h"
#include "zenoh-pico/utils/pointers.h"

WSADATA wsaData;

z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking) {
    u_long mode = blocking ? 0 : 1;  // 0 for blocking mode, 1 for non-blocking mode
    if (ioctlsocket(sock->_sock._fd, FIONBIO, &mode) != 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    return _Z_RES_OK;
}

z_result_t _z_socket_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    struct sockaddr naddr;
    int nlen = sizeof(naddr);
    sock_out->_sock._fd = INVALID_SOCKET;
    SOCKET con_socket = accept(sock_in->_sock._fd, &naddr, &nlen);
    if (con_socket == INVALID_SOCKET) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    // Set socket options
    DWORD tv = Z_CONFIG_SOCKET_TIMEOUT;
    if (setsockopt(con_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        closesocket(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    int flags = 1;
    if (setsockopt(con_socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0) {
        closesocket(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#if Z_FEATURE_TCP_NODELAY == 1
    if (setsockopt(con_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0) {
        closesocket(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#endif
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if (setsockopt(con_socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0) {
        closesocket(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    // Note socket
    sock_out->_sock._fd = con_socket;
    return _Z_RES_OK;
}

void _z_socket_close(_z_sys_net_socket_t *sock) {
    if (sock->_sock._fd != INVALID_SOCKET) {
        shutdown(sock->_sock._fd, SD_BOTH);
        closesocket(sock->_sock._fd);
        sock->_sock._fd = INVALID_SOCKET;
    }
}

z_result_t _z_socket_wait_event(void *v_peers, _z_mutex_rec_t *mutex) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    // Create select mask
    _z_transport_peer_unicast_slist_t **peers = (_z_transport_peer_unicast_slist_t **)v_peers;
    _z_mutex_rec_mt_lock(mutex);
    _z_transport_peer_unicast_slist_t *curr = *peers;
    while (curr != NULL) {
        _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(curr);
        FD_SET(peer->_socket._sock._fd, &read_fds);
        curr = _z_transport_peer_unicast_slist_next(curr);
    }
    _z_mutex_rec_mt_unlock(mutex);
    // Wait for events
    struct timeval timeout;
    timeout.tv_sec = Z_CONFIG_SOCKET_TIMEOUT / 1000;
    timeout.tv_usec = (Z_CONFIG_SOCKET_TIMEOUT % 1000) * 1000;
    int result = select(0, &read_fds, NULL, NULL, &timeout);
    if (result <= 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    // Mark sockets that are pending
    _z_mutex_rec_mt_lock(mutex);
    curr = *peers;
    while (curr != NULL) {
        _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(curr);
        if (FD_ISSET(peer->_socket._sock._fd, &read_fds)) {
            peer->_pending = true;
        }
        curr = _z_transport_peer_unicast_slist_next(curr);
    }
    _z_mutex_rec_mt_unlock(mutex);
    return _Z_RES_OK;
}

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on Windows port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_SERIAL == 1
#error "Serial not supported yet on Windows port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on Windows port of Zenoh-Pico"
#endif
