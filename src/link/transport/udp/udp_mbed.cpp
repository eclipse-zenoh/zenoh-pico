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

#include "zenoh-pico/link/transport/udp_unicast.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_udp_mbed_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
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

static void _z_udp_mbed_endpoint_clear(_z_sys_net_endpoint_t *ep) {
    delete ep->_iptcp;
    ep->_iptcp = NULL;
}

static z_result_t _z_udp_mbed_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    _ZP_UNUSED(endpoint);
    NetworkInterface *iface = NetworkInterface::get_default_instance();
    if (iface == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock->_udp = new UDPSocket();
    if (sock->_udp == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock->_udp->set_timeout((int)tout);
    // flawfinder: ignore
    if (sock->_udp->open(iface) < 0) {
        delete sock->_udp;
        sock->_udp = NULL;
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

static z_result_t _z_udp_mbed_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(endpoint);
    _ZP_UNUSED(tout);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

static void _z_udp_mbed_close(_z_sys_net_socket_t *sock) {
    if (sock->_udp != NULL) {
        sock->_udp->close();
        delete sock->_udp;
        sock->_udp = NULL;
    }
}

static size_t _z_udp_mbed_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    SocketAddress raddr;
    nsapi_size_or_error_t rb = sock._udp->recvfrom(&raddr, ptr, len);
    if (rb < 0) {
        return SIZE_MAX;
    }
    return (size_t)rb;
}

static size_t _z_udp_mbed_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_udp_mbed_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_udp_mbed_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                                const _z_sys_net_endpoint_t endpoint) {
    nsapi_size_or_error_t wb = sock._udp->sendto(*endpoint._iptcp, ptr, len);
    if (wb < 0) {
        return SIZE_MAX;
    }
    return (size_t)wb;
}

z_result_t _z_udp_unicast_endpoint_init(_z_sys_net_endpoint_t *ep, const char *address, const char *port) {
    return _z_udp_mbed_endpoint_init(ep, address, port);
}

void _z_udp_unicast_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_udp_mbed_endpoint_clear(ep); }

z_result_t _z_udp_unicast_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    return _z_udp_mbed_open(sock, endpoint, tout);
}

z_result_t _z_udp_unicast_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    return _z_udp_mbed_listen(sock, endpoint, tout);
}

void _z_udp_unicast_close(_z_sys_net_socket_t *sock) { _z_udp_mbed_close(sock); }

size_t _z_udp_unicast_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_udp_mbed_read(sock, ptr, len);
}

size_t _z_udp_unicast_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_udp_mbed_read_exact(sock, ptr, len);
}

size_t _z_udp_unicast_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                            const _z_sys_net_endpoint_t endpoint) {
    return _z_udp_mbed_write(sock, ptr, len, endpoint);
}
}  // extern "C"

#endif /* defined(ZP_PLATFORM_SOCKET_MBED) */
