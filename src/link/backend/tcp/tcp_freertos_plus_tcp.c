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

#if defined(ZP_PLATFORM_SOCKET_FREERTOS_PLUS_TCP)

#include <stdlib.h>

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_tcp_freertos_plus_tcp_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address,
                                                         const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    if (FreeRTOS_getaddrinfo(s_address, NULL, NULL, &ep->_iptcp) < 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
        return ret;
    }

    ep->_iptcp->ai_addr->sin_family = ep->_iptcp->ai_family;

    uint32_t port = strtoul(s_port, NULL, 10);
    if ((port > (uint32_t)0) && (port <= (uint32_t)65535)) {
        ep->_iptcp->ai_addr->sin_port = FreeRTOS_htons((uint16_t)port);
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static void _z_tcp_freertos_plus_tcp_endpoint_clear(_z_sys_net_endpoint_t *ep) { FreeRTOS_freeaddrinfo(ep->_iptcp); }

static z_result_t _z_tcp_freertos_plus_tcp_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint,
                                                uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_socket = FreeRTOS_socket(endpoint._iptcp->ai_family, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (sock->_socket != FREERTOS_INVALID_SOCKET) {
        TickType_t receive_timeout = pdMS_TO_TICKS(tout);

        if (FreeRTOS_setsockopt(sock->_socket, 0, FREERTOS_SO_RCVTIMEO, &receive_timeout, 0) != 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        } else if (FreeRTOS_connect(sock->_socket, endpoint._iptcp->ai_addr, endpoint._iptcp->ai_addrlen) < 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        if (ret != _Z_RES_OK) {
            FreeRTOS_closesocket(sock->_socket);
            sock->_socket = FREERTOS_INVALID_SOCKET;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_tcp_freertos_plus_tcp_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    sock->_socket = FreeRTOS_socket(endpoint._iptcp->ai_family, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    if (sock->_socket == FREERTOS_INVALID_SOCKET) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    if (FreeRTOS_bind(sock->_socket, endpoint._iptcp->ai_addr, endpoint._iptcp->ai_addrlen) != 0) {
        FreeRTOS_closesocket(sock->_socket);
        sock->_socket = FREERTOS_INVALID_SOCKET;
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    if (FreeRTOS_listen(sock->_socket, Z_LISTEN_MAX_CONNECTION_NB) != 0) {
        FreeRTOS_closesocket(sock->_socket);
        sock->_socket = FREERTOS_INVALID_SOCKET;
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    return _Z_RES_OK;
}

static z_result_t _z_tcp_freertos_plus_tcp_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    struct freertos_sockaddr naddr;
    socklen_t nlen = sizeof(naddr);
    sock_out->_socket = FREERTOS_INVALID_SOCKET;
    Socket_t con_socket = FreeRTOS_accept(sock_in->_socket, &naddr, &nlen);
    if (con_socket == FREERTOS_INVALID_SOCKET) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    TickType_t receive_timeout = pdMS_TO_TICKS(Z_CONFIG_SOCKET_TIMEOUT);
    if (FreeRTOS_setsockopt(con_socket, 0, FREERTOS_SO_RCVTIMEO, &receive_timeout, 0) != 0) {
        FreeRTOS_closesocket(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock_out->_socket = con_socket;
    return _Z_RES_OK;
}

static void _z_tcp_freertos_plus_tcp_close(_z_sys_net_socket_t *sock) {
    if (sock->_socket != FREERTOS_INVALID_SOCKET) {
        FreeRTOS_closesocket(sock->_socket);
        sock->_socket = FREERTOS_INVALID_SOCKET;
    }
}

static size_t _z_tcp_freertos_plus_tcp_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    BaseType_t rb = FreeRTOS_recv(sock._socket, ptr, len, 0);
    if (rb < 0) {
        return SIZE_MAX;
    }

    return (size_t)rb;
}

static size_t _z_tcp_freertos_plus_tcp_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_tcp_freertos_plus_tcp_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_tcp_freertos_plus_tcp_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return (size_t)FreeRTOS_send(sock._socket, ptr, len, 0);
}

z_result_t _z_tcp_endpoint_init(_z_sys_net_endpoint_t *ep, const char *address, const char *port) {
    return _z_tcp_freertos_plus_tcp_endpoint_init(ep, address, port);
}

void _z_tcp_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_tcp_freertos_plus_tcp_endpoint_clear(ep); }

z_result_t _z_tcp_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    return _z_tcp_freertos_plus_tcp_open(sock, endpoint, tout);
}

z_result_t _z_tcp_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    return _z_tcp_freertos_plus_tcp_listen(sock, endpoint);
}

z_result_t _z_tcp_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    return _z_tcp_freertos_plus_tcp_accept(sock_in, sock_out);
}

void _z_tcp_close(_z_sys_net_socket_t *sock) { _z_tcp_freertos_plus_tcp_close(sock); }

size_t _z_tcp_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_tcp_freertos_plus_tcp_read(sock, ptr, len);
}

size_t _z_tcp_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_tcp_freertos_plus_tcp_read_exact(sock, ptr, len);
}

size_t _z_tcp_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return _z_tcp_freertos_plus_tcp_write(sock, ptr, len);
}

#endif /* defined(ZP_PLATFORM_SOCKET_FREERTOS_PLUS_TCP) */
