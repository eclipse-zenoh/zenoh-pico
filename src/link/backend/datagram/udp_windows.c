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

#include "zenoh-pico/link/backend/datagram.h"

#if defined(ZENOH_WINDOWS)

#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static WSADATA _z_udp_windows_wsa_data;

static z_result_t _z_udp_windows_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &_z_udp_windows_wsa_data) == 0) {
        ADDRINFOA hints;
        (void)memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = 0;
        hints.ai_protocol = IPPROTO_UDP;

        if (getaddrinfo(s_address, s_port, &hints, &ep->_ep._iptcp) != 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static void _z_udp_windows_endpoint_clear(_z_sys_net_endpoint_t *ep) {
    freeaddrinfo(ep->_ep._iptcp);
    WSACleanup();
}

static z_result_t _z_udp_windows_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &_z_udp_windows_wsa_data) != 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    sock->_sock._fd =
        socket(endpoint._ep._iptcp->ai_family, endpoint._ep._iptcp->ai_socktype, endpoint._ep._iptcp->ai_protocol);
    if (sock->_sock._fd != INVALID_SOCKET) {
        DWORD tv = tout;
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
        if (ret != _Z_RES_OK) {
            closesocket(sock->_sock._fd);
            sock->_sock._fd = INVALID_SOCKET;
            WSACleanup();
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_udp_windows_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint,
                                        uint32_t tout) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(endpoint);
    _ZP_UNUSED(tout);
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    return _Z_ERR_GENERIC;
}

static void _z_udp_windows_close(_z_sys_net_socket_t *sock) {
    if (sock->_sock._fd != INVALID_SOCKET) {
        closesocket(sock->_sock._fd);
        sock->_sock._fd = INVALID_SOCKET;
        WSACleanup();
    }
}

static size_t _z_udp_windows_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    SOCKADDR_STORAGE raddr;
    int addrlen = sizeof(raddr);
    int rb = recvfrom(sock._sock._fd, (char *)ptr, (int)len, 0, (SOCKADDR *)&raddr, &addrlen);
    if (rb == SOCKET_ERROR) {
        return SIZE_MAX;
    }

    return (size_t)rb;
}

static size_t _z_udp_windows_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_udp_windows_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_udp_windows_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t endpoint) {
    int wb = sendto(sock._sock._fd, (const char *)ptr, (int)len, 0, endpoint._ep._iptcp->ai_addr,
                    (int)endpoint._ep._iptcp->ai_addrlen);
    if (wb == SOCKET_ERROR) {
        return SIZE_MAX;
    }

    return (size_t)wb;
}

const _z_datagram_ops_t _z_udp_windows_datagram_ops = {
    .endpoint_init = _z_udp_windows_endpoint_init,
    .endpoint_clear = _z_udp_windows_endpoint_clear,
    .open = _z_udp_windows_open,
    .listen = _z_udp_windows_listen,
    .close = _z_udp_windows_close,
    .read = _z_udp_windows_read,
    .read_exact = _z_udp_windows_read_exact,
    .write = _z_udp_windows_write,
};

#endif /* defined(ZENOH_WINDOWS) */
