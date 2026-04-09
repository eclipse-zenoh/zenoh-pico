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

#if defined(ZP_PLATFORM_SOCKET_MBED) && (Z_FEATURE_LINK_UDP_MULTICAST == 1)

#include <NetworkInterface.h>
#include <mbed.h>

extern "C" {
#include <string.h>

#include "zenoh-pico/link/backend/udp_multicast.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

z_result_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                 uint32_t tout, const char *iface) {
    z_result_t ret = _Z_RES_OK;
    _ZP_UNUSED(rep);
    (void)(lep);
    (void)(iface);

    sock->_udp = new UDPSocket();
    sock->_udp->set_timeout(tout);
    // flawfinder: ignore
    if ((ret == _Z_RES_OK) && (sock->_udp->open(NetworkInterface::get_default_instance()) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete sock->_udp;
        sock->_udp = NULL;
    }

    return ret;
}

z_result_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                   const char *iface, const char *join) {
    (void)iface;
    (void)join;
    z_result_t ret = _Z_RES_OK;

    sock->_udp = new UDPSocket();
    sock->_udp->set_timeout(tout);
    // flawfinder: ignore
    if ((ret == _Z_RES_OK) && (sock->_udp->open(NetworkInterface::get_default_instance()) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if ((ret == _Z_RES_OK) && (sock->_udp->bind(rep._iptcp->get_port()) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if ((ret == _Z_RES_OK) && (sock->_udp->join_multicast_group(*rep._iptcp) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        sock->_udp->close();
        delete sock->_udp;
        sock->_udp = NULL;
    }

    return ret;
}

void _z_close_udp_multicast(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                            const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep) {
    _ZP_UNUSED(lep);
    if (sockrecv->_udp != NULL) {
        sockrecv->_udp->leave_multicast_group(*rep._iptcp);
        sockrecv->_udp->close();
        delete sockrecv->_udp;
        sockrecv->_udp = NULL;
    }

    if (socksend->_udp != NULL) {
        socksend->_udp->close();
        delete socksend->_udp;
        socksend->_udp = NULL;
    }
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *addr) {
    _ZP_UNUSED(lep);
    SocketAddress raddr;
    nsapi_size_or_error_t rb = 0;

    do {
        rb = sock._udp->recvfrom(&raddr, ptr, len);
        if (rb < (nsapi_size_or_error_t)0) {
            rb = SIZE_MAX;
            break;
        }

        if (raddr.get_ip_version() == NSAPI_IPv4) {
            addr->len = NSAPI_IPv4_BYTES + sizeof(uint16_t);
            // flawfinder: ignore
            (void)memcpy(const_cast<uint8_t *>(addr->start), raddr.get_ip_bytes(), NSAPI_IPv4_BYTES);
            uint16_t port = raddr.get_port();
            // flawfinder: ignore
            (void)memcpy(const_cast<uint8_t *>(addr->start + NSAPI_IPv4_BYTES), &port, sizeof(uint16_t));
            break;
        } else if (raddr.get_ip_version() == NSAPI_IPv6) {
            addr->len = NSAPI_IPv6_BYTES + sizeof(uint16_t);
            // flawfinder: ignore
            (void)memcpy(const_cast<uint8_t *>(addr->start), raddr.get_ip_bytes(), NSAPI_IPv6_BYTES);
            uint16_t port = raddr.get_port();
            // flawfinder: ignore
            (void)memcpy(const_cast<uint8_t *>(addr->start + NSAPI_IPv6_BYTES), &port, sizeof(uint16_t));
            break;
        } else {
            continue;
        }
    } while (1);

    return rb;
}

size_t _z_read_exact_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t lep, _z_slice_t *addr) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_udp_multicast(sock, pos, len - n, lep, addr);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

size_t _z_send_udp_multicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                             const _z_sys_net_endpoint_t rep) {
    return sock._udp->sendto(*rep._iptcp, ptr, len);
}

z_result_t _z_udp_multicast_endpoint_init_from_address(_z_sys_net_endpoint_t *ep, const _z_string_t *address) {
    return _z_udp_multicast_default_endpoint_init_from_address(ep, address);
}

void _z_udp_multicast_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_udp_multicast_default_endpoint_clear(ep); }

z_result_t _z_udp_multicast_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                 uint32_t tout, const char *iface) {
    return _z_open_udp_multicast(sock, rep, lep, tout, iface);
}

z_result_t _z_udp_multicast_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                   const char *iface, const char *join) {
    return _z_listen_udp_multicast(sock, rep, tout, iface, join);
}

void _z_udp_multicast_close(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                            const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep) {
    _z_close_udp_multicast(sockrecv, socksend, rep, lep);
}

size_t _z_udp_multicast_read_exact(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t lep, _z_slice_t *ep) {
    return _z_read_exact_udp_multicast(sock, ptr, len, lep, ep);
}

size_t _z_udp_multicast_read(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *ep) {
    return _z_read_udp_multicast(sock, ptr, len, lep, ep);
}

size_t _z_udp_multicast_write(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                              const _z_sys_net_endpoint_t rep) {
    return _z_send_udp_multicast(sock, ptr, len, rep);
}

}  // extern "C"

#endif
