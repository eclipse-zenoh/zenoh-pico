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

#include "zenoh-pico/link/backend/udp_unicast.h"

#if defined(ZP_PLATFORM_SOCKET_FREERTOS_PLUS_TCP)

#include <stdlib.h>

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_udp_freertos_plus_tcp_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address,
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

static void _z_udp_freertos_plus_tcp_endpoint_clear(_z_sys_net_endpoint_t *ep) { FreeRTOS_freeaddrinfo(ep->_iptcp); }

static z_result_t _z_udp_freertos_plus_tcp_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint,
                                                uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_socket = FreeRTOS_socket(endpoint._iptcp->ai_family, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
    if (sock->_socket != FREERTOS_INVALID_SOCKET) {
        TickType_t receive_timeout = pdMS_TO_TICKS(tout);

        if (FreeRTOS_setsockopt(sock->_socket, 0, FREERTOS_SO_RCVTIMEO, &receive_timeout, 0) != 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            FreeRTOS_closesocket(sock->_socket);
            sock->_socket = FREERTOS_INVALID_SOCKET;
            ret = _Z_ERR_GENERIC;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_udp_freertos_plus_tcp_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint,
                                                  uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_socket = FreeRTOS_socket(endpoint._iptcp->ai_family, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP);
    if (sock->_socket != FREERTOS_INVALID_SOCKET) {
        TickType_t receive_timeout = pdMS_TO_TICKS(tout);

        if (FreeRTOS_setsockopt(sock->_socket, 0, FREERTOS_SO_RCVTIMEO, &receive_timeout, 0) != 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        } else if (FreeRTOS_bind(sock->_socket, endpoint._iptcp->ai_addr, endpoint._iptcp->ai_addrlen) != 0) {
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

static void _z_udp_freertos_plus_tcp_close(_z_sys_net_socket_t *sock) {
    if (sock->_socket != FREERTOS_INVALID_SOCKET) {
        FreeRTOS_closesocket(sock->_socket);
        sock->_socket = FREERTOS_INVALID_SOCKET;
    }
}

static size_t _z_udp_freertos_plus_tcp_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    struct freertos_sockaddr raddr;
    uint32_t addrlen = sizeof(struct freertos_sockaddr);

    int32_t rb = FreeRTOS_recvfrom(sock._socket, ptr, len, 0, &raddr, &addrlen);
    if (rb < 0) {
        return SIZE_MAX;
    }

    return (size_t)rb;
}

static size_t _z_udp_freertos_plus_tcp_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_udp_freertos_plus_tcp_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_udp_freertos_plus_tcp_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                                             const _z_sys_net_endpoint_t endpoint) {
    return (size_t)FreeRTOS_sendto(sock._socket, ptr, len, 0, endpoint._iptcp->ai_addr, endpoint._iptcp->ai_addrlen);
}

const _z_udp_unicast_ops_t _z_udp_freertos_plus_tcp_unicast_ops = {
    .endpoint_init = _z_udp_freertos_plus_tcp_endpoint_init,
    .endpoint_clear = _z_udp_freertos_plus_tcp_endpoint_clear,
    .open = _z_udp_freertos_plus_tcp_open,
    .listen = _z_udp_freertos_plus_tcp_listen,
    .close = _z_udp_freertos_plus_tcp_close,
    .read = _z_udp_freertos_plus_tcp_read,
    .read_exact = _z_udp_freertos_plus_tcp_read_exact,
    .write = _z_udp_freertos_plus_tcp_write,
};

#endif /* defined(ZP_PLATFORM_SOCKET_FREERTOS_PLUS_TCP) */
