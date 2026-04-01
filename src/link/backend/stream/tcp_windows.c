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

#include "zenoh-pico/link/backend/stream.h"

#if defined(ZP_PLATFORM_SOCKET_WINDOWS)

#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static WSADATA _z_tcp_windows_wsa_data;

static z_result_t _z_tcp_windows_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &_z_tcp_windows_wsa_data) == 0) {
        ADDRINFOA hints;
        ep->_ep._iptcp = NULL;
        (void)memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        hints.ai_protocol = IPPROTO_TCP;

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

static void _z_tcp_windows_endpoint_clear(_z_sys_net_endpoint_t *ep) {
    freeaddrinfo(ep->_ep._iptcp);
    WSACleanup();
}

static z_result_t _z_tcp_windows_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &_z_tcp_windows_wsa_data) != 0) {
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

        int flags = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&flags, sizeof(flags)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

#if Z_FEATURE_TCP_NODELAY == 1
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_sock._fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&flags, sizeof(flags)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
#endif

        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = Z_TRANSPORT_LEASE / 1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_LINGER, (const char *)&ling, sizeof(ling)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        ADDRINFOA *it = NULL;
        for (it = endpoint._ep._iptcp; it != NULL; it = it->ai_next) {
            if ((ret == _Z_RES_OK) && (connect(sock->_sock._fd, it->ai_addr, (int)it->ai_addrlen) < 0)) {
                if (it->ai_next == NULL) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                    break;
                }
            } else {
                break;
            }
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

static z_result_t _z_tcp_windows_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    z_result_t ret = _Z_RES_OK;

    if (WSAStartup(MAKEWORD(2, 2), &_z_tcp_windows_wsa_data) != 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock->_sock._fd =
        socket(endpoint._ep._iptcp->ai_family, endpoint._ep._iptcp->ai_socktype, endpoint._ep._iptcp->ai_protocol);
    if (sock->_sock._fd == INVALID_SOCKET) {
        WSACleanup();
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    int value = 1;
    if ((ret == _Z_RES_OK) &&
        (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&value, sizeof(value)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    int flags = 1;
    if ((ret == _Z_RES_OK) &&
        (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&flags, sizeof(flags)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

#if Z_FEATURE_TCP_NODELAY == 1
    if ((ret == _Z_RES_OK) &&
        (setsockopt(sock->_sock._fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&flags, sizeof(flags)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }
#endif

    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if ((ret == _Z_RES_OK) &&
        (setsockopt(sock->_sock._fd, SOL_SOCKET, SO_LINGER, (const char *)&ling, sizeof(ling)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        closesocket(sock->_sock._fd);
        WSACleanup();
        return ret;
    }

    ADDRINFOA *it = NULL;
    for (it = endpoint._ep._iptcp; it != NULL; it = it->ai_next) {
        if (bind(sock->_sock._fd, it->ai_addr, (int)it->ai_addrlen) < 0) {
            if (it->ai_next == NULL) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
                break;
            }
        }
        if (listen(sock->_sock._fd, Z_LISTEN_MAX_CONNECTION_NB) < 0) {
            if (it->ai_next == NULL) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
                break;
            }
        }
    }

    if (ret != _Z_RES_OK) {
        closesocket(sock->_sock._fd);
        sock->_sock._fd = INVALID_SOCKET;
    }

    WSACleanup();
    return ret;
}

static z_result_t _z_tcp_windows_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    struct sockaddr naddr;
    int nlen = sizeof(naddr);
    sock_out->_sock._fd = INVALID_SOCKET;
    SOCKET con_socket = accept(sock_in->_sock._fd, &naddr, &nlen);
    if (con_socket == INVALID_SOCKET) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

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

    sock_out->_sock._fd = con_socket;
    return _Z_RES_OK;
}

static void _z_tcp_windows_close(_z_sys_net_socket_t *sock) {
    if (sock->_sock._fd != INVALID_SOCKET) {
        shutdown(sock->_sock._fd, SD_BOTH);
        closesocket(sock->_sock._fd);
        sock->_sock._fd = INVALID_SOCKET;
        WSACleanup();
    }
}

static size_t _z_tcp_windows_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    int rb = recv(sock._sock._fd, (char *)ptr, (int)len, 0);
    if (rb == SOCKET_ERROR) {
        return SIZE_MAX;
    }

    return (size_t)rb;
}

static size_t _z_tcp_windows_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_tcp_windows_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_tcp_windows_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    int wb = send(sock._sock._fd, (const char *)ptr, (int)len, 0);
    if (wb == SOCKET_ERROR) {
        return SIZE_MAX;
    }

    return (size_t)wb;
}

const _z_stream_ops_t _z_tcp_windows_stream_ops = {
    .endpoint_init = _z_tcp_windows_endpoint_init,
    .endpoint_clear = _z_tcp_windows_endpoint_clear,
    .open = _z_tcp_windows_open,
    .listen = _z_tcp_windows_listen,
    .accept = _z_tcp_windows_accept,
    .close = _z_tcp_windows_close,
    .read = _z_tcp_windows_read,
    .read_exact = _z_tcp_windows_read_exact,
    .write = _z_tcp_windows_write,
};

#endif /* defined(ZP_PLATFORM_SOCKET_WINDOWS) */
