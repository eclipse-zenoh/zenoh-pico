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

#include <winsock2.h>
// The following includes must come after winsock2
#include <iphlpapi.h>
#include <stdio.h>
#include <ws2tcpip.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/backend/socket.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

WSADATA wsaData;

static uintptr_t _z_socket_id_impl(const _z_sys_net_socket_t *sock) { return (uintptr_t)sock->_sock._fd; }

z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking) {
    u_long mode = blocking ? 0 : 1;
    if (ioctlsocket(sock->_sock._fd, FIONBIO, &mode) != 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    return _Z_RES_OK;
}

z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) { return _z_socket_set_blocking(sock, false); }

void _z_socket_close(_z_sys_net_socket_t *sock) {
    if (sock->_sock._fd != INVALID_SOCKET) {
        shutdown(sock->_sock._fd, SD_BOTH);
        closesocket(sock->_sock._fd);
        sock->_sock._fd = INVALID_SOCKET;
    }
}

static z_result_t _z_socket_wait_readable_impl(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready,
                                               uint32_t timeout_ms) {
    fd_set read_fds;
    size_t i = 0;

    FD_ZERO(&read_fds);
    if (count == 0) {
        return _Z_RES_OK;
    }

    for (i = 0; i < count; i++) {
        ready[i] = 0;
        FD_SET(sockets[i]._sock._fd, &read_fds);
    }

    struct timeval timeout = {
        .tv_sec = (long)(timeout_ms / 1000U),
        .tv_usec = (long)((timeout_ms % 1000U) * 1000U),
    };
    int result = select(0, &read_fds, NULL, NULL, &timeout);
    if (result <= 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    for (i = 0; i < count; i++) {
        if (FD_ISSET(sockets[i]._sock._fd, &read_fds)) {
            ready[i] = 1;
        }
    }

    return _Z_RES_OK;
}

uintptr_t _z_socket_id(const _z_sys_net_socket_t *sock) { return _z_socket_id_impl(sock); }

z_result_t _z_socket_wait_readable(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready,
                                   uint32_t timeout_ms) {
    return _z_socket_wait_readable_impl(sockets, count, ready, timeout_ms);
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
