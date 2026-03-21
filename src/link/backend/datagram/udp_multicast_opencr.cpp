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

#include "zenoh-pico/config.h"

#if defined(ZENOH_ARDUINO_OPENCR) && (Z_FEATURE_LINK_UDP_MULTICAST == 1)

#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

extern "C" {
#include "zenoh-pico/link/backend/udp_multicast.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

z_result_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                 uint32_t tout, const char *iface) {
    z_result_t ret = _Z_RES_OK;
    (void)(rep);
    (void)(tout);
    (void)(iface);

    sock->_udp = new WiFiUDP();
    if (!sock->_udp->begin(55555)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    lep->_iptcp._addr = new IPAddress();

    if (ret != _Z_RES_OK) {
        delete sock->_udp;
        sock->_udp = NULL;
        delete lep->_iptcp._addr;
        lep->_iptcp._addr = NULL;
    }

    return ret;
}

z_result_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                                   const char *iface, const char *join) {
    (void)tout;
    (void)iface;
    (void)join;
    z_result_t ret = _Z_RES_OK;

    sock->_udp = new WiFiUDP();
    if (!sock->_udp->beginMulticast(*rep._iptcp._addr, rep._iptcp._port)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete sock->_udp;
        sock->_udp = NULL;
    }

    return ret;
}

void _z_close_udp_multicast(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                            const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep) {
    _ZP_UNUSED(rep);
    _ZP_UNUSED(lep);

    if (sockrecv->_udp != NULL) {
        sockrecv->_udp->stop();
        delete sockrecv->_udp;
        sockrecv->_udp = NULL;
    }

    if (socksend->_udp != NULL) {
        socksend->_udp->stop();
        delete socksend->_udp;
        socksend->_udp = NULL;
    }
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *addr) {
    _ZP_UNUSED(lep);
    ssize_t rb = 0;
    do {
        rb = sock._udp->parsePacket();
    } while (rb == 0);

    if (rb <= (ssize_t)len) {
        // flawfinder: ignore
        if (sock._udp->read(ptr, rb) == rb) {
            if (addr != NULL) {
                IPAddress rip = sock._udp->remoteIP();
                uint16_t rport = sock._udp->remotePort();
                addr->len = 4U + sizeof(uint16_t);

                uint8_t *dst = const_cast<uint8_t *>(addr->start);
                dst[0] = rip[0];
                dst[1] = rip[1];
                dst[2] = rip[2];
                dst[3] = rip[3];

                const uint8_t *port_bytes = (const uint8_t *)&rport;
                dst[4] = port_bytes[0];
                dst[5] = port_bytes[1];
            }
        } else {
            rb = 0;
        }
    }

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
    sock._udp->beginPacket(*rep._iptcp._addr, rep._iptcp._port);
    sock._udp->write(ptr, len);
    sock._udp->endPacket();

    return len;
}

const _z_udp_multicast_ops_t _z_udp_multicast_ops = {
    .endpoint_init_from_address = _z_udp_multicast_default_endpoint_init_from_address,
    .endpoint_clear = _z_udp_multicast_default_endpoint_clear,
    .open = _z_open_udp_multicast,
    .listen = _z_listen_udp_multicast,
    .close = _z_close_udp_multicast,
    .read_exact = _z_read_exact_udp_multicast,
    .read = _z_read_udp_multicast,
    .write = _z_send_udp_multicast,
};

}  // extern "C"

#endif
