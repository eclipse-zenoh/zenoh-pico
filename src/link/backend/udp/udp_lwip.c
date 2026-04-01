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

#if defined(ZP_PLATFORM_SOCKET_LWIP) && (Z_FEATURE_LINK_UDP_UNICAST == 1)

#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "zenoh-pico/link/backend/lwip_socket.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_udp_lwip_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct addrinfo hints;
    ep->_iptcp = NULL;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(s_address, s_port, &hints, &ep->_iptcp) != 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    } else if (ep->_iptcp != NULL && ep->_iptcp->ai_addr != NULL) {
        ep->_iptcp->ai_addr->sa_family = ep->_iptcp->ai_family;
    }

    return ret;
}

static void _z_udp_lwip_endpoint_clear(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }

static z_result_t _z_udp_lwip_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    _z_lwip_socket_set(sock,
                       socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol));
    if (_z_lwip_socket_get(*sock) != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(_z_lwip_socket_get(*sock), SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        if (ret != _Z_RES_OK) {
            close(_z_lwip_socket_get(*sock));
            _z_lwip_socket_set(sock, -1);
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_udp_lwip_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(endpoint);
    _ZP_UNUSED(tout);
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    return _Z_ERR_GENERIC;
}

static void _z_udp_lwip_close(_z_sys_net_socket_t *sock) {
    if (_z_lwip_socket_get(*sock) >= 0) {
        close(_z_lwip_socket_get(*sock));
        _z_lwip_socket_set(sock, -1);
    }
}

static size_t _z_udp_lwip_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    struct sockaddr_storage raddr;
    unsigned int addrlen = sizeof(struct sockaddr_storage);

    ssize_t rb = recvfrom(_z_lwip_socket_get(sock), ptr, len, 0, (struct sockaddr *)&raddr, &addrlen);
    if (rb < (ssize_t)0) {
        return SIZE_MAX;
    }
    return (size_t)rb;
}

static size_t _z_udp_lwip_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_udp_lwip_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, (ptrdiff_t)rb);
    } while (n != len);

    return n;
}

static size_t _z_udp_lwip_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                                const _z_sys_net_endpoint_t endpoint) {
    return (size_t)sendto(_z_lwip_socket_get(sock), ptr, len, 0, endpoint._iptcp->ai_addr, endpoint._iptcp->ai_addrlen);
}

const _z_udp_unicast_ops_t _z_udp_lwip_unicast_ops = {
    .endpoint_init = _z_udp_lwip_endpoint_init,
    .endpoint_clear = _z_udp_lwip_endpoint_clear,
    .open = _z_udp_lwip_open,
    .listen = _z_udp_lwip_listen,
    .close = _z_udp_lwip_close,
    .read = _z_udp_lwip_read,
    .read_exact = _z_udp_lwip_read_exact,
    .write = _z_udp_lwip_write,
};

#endif /* defined(ZP_PLATFORM_SOCKET_LWIP) */
