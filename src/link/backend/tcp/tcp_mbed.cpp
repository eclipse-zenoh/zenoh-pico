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

#include "zenoh-pico/system/platform.h"

#if defined(ZP_PLATFORM_SOCKET_MBED)

#include <NetworkInterface.h>
#include <mbed.h>

extern "C" {
#include <stddef.h>
#include <stdlib.h>

#include "zenoh-pico/link/backend/tcp.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_tcp_mbed_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    uint32_t port = strtoul(s_port, NULL, 10);
    if ((port == 0U) || (port > 65535U)) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    ep->_iptcp = new SocketAddress(s_address, port);
    if (ep->_iptcp == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

static void _z_tcp_mbed_endpoint_clear(_z_sys_net_endpoint_t *ep) {
    delete ep->_iptcp;
    ep->_iptcp = NULL;
}

static z_result_t _z_tcp_mbed_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    NetworkInterface *iface = NetworkInterface::get_default_instance();
    if (iface == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock->_tcp = new TCPSocket();
    if (sock->_tcp == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock->_tcp->set_timeout((int)tout);
    // flawfinder: ignore
    if (sock->_tcp->open(iface) < 0) {
        delete sock->_tcp;
        sock->_tcp = NULL;
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    if (sock->_tcp->connect(*endpoint._iptcp) < 0) {
        sock->_tcp->close();
        delete sock->_tcp;
        sock->_tcp = NULL;
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

static z_result_t _z_tcp_mbed_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(endpoint);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

static z_result_t _z_tcp_mbed_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    _ZP_UNUSED(sock_in);
    _ZP_UNUSED(sock_out);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

static void _z_tcp_mbed_close(_z_sys_net_socket_t *sock) {
    if (sock->_tcp != NULL) {
        sock->_tcp->close();
        delete sock->_tcp;
        sock->_tcp = NULL;
    }
}

static size_t _z_tcp_mbed_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    nsapi_size_or_error_t rb = sock._tcp->recv(ptr, len);
    if (rb < 0) {
        return SIZE_MAX;
    }
    return (size_t)rb;
}

static size_t _z_tcp_mbed_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_tcp_mbed_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_tcp_mbed_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    nsapi_size_or_error_t wb = sock._tcp->send(ptr, len);
    if (wb < 0) {
        return SIZE_MAX;
    }
    return (size_t)wb;
}

z_result_t _z_tcp_endpoint_init(_z_sys_net_endpoint_t *ep, const char *address, const char *port) {
    return _z_tcp_mbed_endpoint_init(ep, address, port);
}

void _z_tcp_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_tcp_mbed_endpoint_clear(ep); }

z_result_t _z_tcp_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    return _z_tcp_mbed_open(sock, endpoint, tout);
}

z_result_t _z_tcp_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    return _z_tcp_mbed_listen(sock, endpoint);
}

z_result_t _z_tcp_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    return _z_tcp_mbed_accept(sock_in, sock_out);
}

void _z_tcp_close(_z_sys_net_socket_t *sock) { _z_tcp_mbed_close(sock); }

size_t _z_tcp_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) { return _z_tcp_mbed_read(sock, ptr, len); }

size_t _z_tcp_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_tcp_mbed_read_exact(sock, ptr, len);
}

size_t _z_tcp_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return _z_tcp_mbed_write(sock, ptr, len);
}
}  // extern "C"

#endif /* defined(ZP_PLATFORM_SOCKET_MBED) */
