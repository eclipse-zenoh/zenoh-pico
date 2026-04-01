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

#if defined(ZP_PLATFORM_SOCKET_OPENCR)

#include <Arduino.h>
#include <WiFiClient.h>
#include <stddef.h>
#include <stdlib.h>

extern "C" {
#include "zenoh-pico/link/backend/stream.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_tcp_opencr_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    ep->_iptcp._addr = new IPAddress();
    if (!ep->_iptcp._addr->fromString(s_address)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    ep->_iptcp._port = strtoul(s_port, NULL, 10);
    if ((ep->_iptcp._port < (uint32_t)1) ||
        (ep->_iptcp._port > (uint32_t)65535)) {  // Port numbers should range from 1 to 65535
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete ep->_iptcp._addr;
        ep->_iptcp._addr = NULL;
    }

    return ret;
}

static void _z_tcp_opencr_endpoint_clear(_z_sys_net_endpoint_t *ep) {
    delete ep->_iptcp._addr;
    ep->_iptcp._addr = NULL;
}

static z_result_t _z_tcp_opencr_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    _ZP_UNUSED(tout);
    z_result_t ret = _Z_RES_OK;

    sock->_tcp = new WiFiClient();
    if (!sock->_tcp->connect(*endpoint._iptcp._addr, endpoint._iptcp._port)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete sock->_tcp;
        sock->_tcp = NULL;
    }

    return ret;
}

static z_result_t _z_tcp_opencr_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(endpoint);
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    return _Z_ERR_GENERIC;
}

static z_result_t _z_tcp_opencr_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    _ZP_UNUSED(sock_in);
    _ZP_UNUSED(sock_out);
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    return _Z_ERR_GENERIC;
}

static void _z_tcp_opencr_close(_z_sys_net_socket_t *sock) {
    if (sock->_tcp != NULL) {
        sock->_tcp->stop();
        delete sock->_tcp;
        sock->_tcp = NULL;
    }
}

static size_t _z_tcp_opencr_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    if (sock._tcp->available() > 0) {
        // flawfinder: ignore
        int rb = sock._tcp->read(ptr, len);
        if (rb < 0) {
            return SIZE_MAX;
        }
        return (size_t)rb;
    }
    return 0;
}

static size_t _z_tcp_opencr_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_tcp_opencr_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_tcp_opencr_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    sock._tcp->write(ptr, len);
    return len;
}

extern const _z_stream_ops_t _z_tcp_opencr_stream_ops = {
    _z_tcp_opencr_endpoint_init, _z_tcp_opencr_endpoint_clear, _z_tcp_opencr_open,
    _z_tcp_opencr_listen,        _z_tcp_opencr_accept,         _z_tcp_opencr_close,
    _z_tcp_opencr_read,          _z_tcp_opencr_read_exact,     _z_tcp_opencr_write,
};
}  // extern "C"

#endif /* defined(ZP_PLATFORM_SOCKET_OPENCR) */
