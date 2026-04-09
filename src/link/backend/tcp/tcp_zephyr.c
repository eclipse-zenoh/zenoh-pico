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

#if defined(ZP_PLATFORM_SOCKET_ZEPHYR) && (Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_TLS == 1 || Z_FEATURE_LINK_WS == 1)

#include <netdb.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <zephyr/net/socket.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_tcp_zephyr_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct zsock_addrinfo hints;
    ep->_iptcp = NULL;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_address, s_port, &hints, &ep->_iptcp) != 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static void _z_tcp_zephyr_endpoint_clear(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }

static z_result_t _z_tcp_zephyr_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_fd = socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol);
    if (sock->_fd != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            /* Zephyr may reject this option depending on the network stack configuration. */
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
        }

#if Z_FEATURE_TCP_NODELAY == 1
        int optflag = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&optflag, sizeof(optflag)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
#endif

#if LWIP_SO_LINGER == 1
        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = Z_TRANSPORT_LEASE / 1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
#endif

        for (struct zsock_addrinfo *it = endpoint._iptcp; it != NULL; it = it->ai_next) {
            if ((ret == _Z_RES_OK) && (connect(sock->_fd, it->ai_addr, it->ai_addrlen) < 0)) {
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
            close(sock->_fd);
            sock->_fd = -1;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_tcp_zephyr_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    sock->_fd = socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol);
    if (sock->_fd == -1) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

#if Z_FEATURE_TCP_NODELAY == 1
    int optflag = 1;
    if (setsockopt(sock->_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&optflag, sizeof(optflag)) < 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        close(sock->_fd);
        sock->_fd = -1;
        return _Z_ERR_GENERIC;
    }
#endif

    for (struct zsock_addrinfo *it = endpoint._iptcp; it != NULL; it = it->ai_next) {
        if (bind(sock->_fd, it->ai_addr, it->ai_addrlen) < 0) {
            if (it->ai_next != NULL) {
                continue;
            }

            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            break;
        }

        if (listen(sock->_fd, Z_LISTEN_MAX_CONNECTION_NB) < 0) {
            if (it->ai_next != NULL) {
                continue;
            }

            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            break;
        }

        return _Z_RES_OK;
    }

    close(sock->_fd);
    sock->_fd = -1;
    return _Z_ERR_GENERIC;
}

static z_result_t _z_tcp_zephyr_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    struct sockaddr naddr;
    unsigned int nlen = sizeof(naddr);
    sock_out->_fd = -1;
    int con_socket = accept(sock_in->_fd, &naddr, &nlen);
    if (con_socket < 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

#if Z_FEATURE_TCP_NODELAY == 1
    int optflag = 1;
    if (setsockopt(con_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&optflag, sizeof(optflag)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#endif
#if LWIP_SO_LINGER == 1
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if (setsockopt(con_socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#endif

    sock_out->_fd = con_socket;
    return _Z_RES_OK;
}

static void _z_tcp_zephyr_close(_z_sys_net_socket_t *sock) {
    if (sock->_fd >= 0) {
        close(sock->_fd);
        sock->_fd = -1;
    }
}

static size_t _z_tcp_zephyr_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    ssize_t rb = recv(sock._fd, ptr, len, 0);
    if (rb < (ssize_t)0) {
        return SIZE_MAX;
    }

    return (size_t)rb;
}

static size_t _z_tcp_zephyr_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_tcp_zephyr_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_tcp_zephyr_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return (size_t)send(sock._fd, ptr, len, 0);
}

z_result_t _z_tcp_endpoint_init(_z_sys_net_endpoint_t *ep, const char *address, const char *port) {
    return _z_tcp_zephyr_endpoint_init(ep, address, port);
}

void _z_tcp_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_tcp_zephyr_endpoint_clear(ep); }

z_result_t _z_tcp_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    return _z_tcp_zephyr_open(sock, endpoint, tout);
}

z_result_t _z_tcp_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    return _z_tcp_zephyr_listen(sock, endpoint);
}

z_result_t _z_tcp_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    return _z_tcp_zephyr_accept(sock_in, sock_out);
}

void _z_tcp_close(_z_sys_net_socket_t *sock) { _z_tcp_zephyr_close(sock); }

size_t _z_tcp_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) { return _z_tcp_zephyr_read(sock, ptr, len); }

size_t _z_tcp_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_tcp_zephyr_read_exact(sock, ptr, len);
}

size_t _z_tcp_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return _z_tcp_zephyr_write(sock, ptr, len);
}

#endif
