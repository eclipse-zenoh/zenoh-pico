//
// Copyright (c) 2022 ZettaScale Technology
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

#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

extern "C" {
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_FEATURE_LINK_TCP == 1

/*------------------ UDP sockets ------------------*/
int8_t _z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    int8_t ret = _Z_RES_OK;

    // Parse and check the validity of the IP address
    ep->_iptcp._addr = new IPAddress();
    if (!ep->_iptcp._addr->fromString(s_address)) {
        ret = _Z_ERR_GENERIC;
    }

    // Parse and check the validity of the port
    ep->_iptcp._port = strtoul(s_port, NULL, 10);
    if ((ep->_iptcp._port < (uint32_t)1) ||
        (ep->_iptcp._port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete ep->_iptcp._addr;
    }

    return ret;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep) { delete ep->_iptcp._addr; }

int8_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    int8_t ret = _Z_RES_OK;

    sock->_tcp = new WiFiClient();
    if (!sock->_tcp->connect(*rep._iptcp._addr, rep._iptcp._port)) {
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete sock->_tcp;
    }

    return ret;
}

int8_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    int8_t ret = _Z_RES_OK;
    (void)sock;
    (void)lep;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_tcp(_z_sys_net_socket_t *sock) {
    sock->_tcp->stop();
    delete sock->_tcp;
}

size_t _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t sb = 0;
    if (sock._tcp->available() > 0) {
        sb = sock._tcp->read(ptr, len);
    }
    return sb;
}

size_t _z_read_exact_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_tcp(sock, pos, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    sock._tcp->write(ptr, len);
    return len;
}

#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1

/*------------------ UDP sockets ------------------*/
int8_t _z_create_endpoint_udp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    int8_t ret = _Z_RES_OK;

    // Parse and check the validity of the IP address
    ep->_iptcp._addr = new IPAddress();
    if (!ep->_iptcp._addr->fromString(s_address)) {
        ret = _Z_ERR_GENERIC;
    }

    // Parse and check the validity of the port
    ep->_iptcp._port = strtoul(s_port, NULL, 10);
    if ((ep->_iptcp._port < (uint32_t)1) ||
        (ep->_iptcp._port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete ep->_iptcp._addr;
    }

    return ret;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t *ep) { delete ep->_iptcp._addr; }
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1

int8_t _z_open_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    int8_t ret = _Z_RES_OK;
    (void)sock;
    (void)rep;

    // FIXME: make it random
    sock->_udp = new WiFiUDP();
    if (!sock->_udp->begin(7447)) {
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete sock->_udp;
    }

    return ret;
}

int8_t _z_listen_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep, uint32_t tout) {
    int8_t ret = _Z_RES_OK;
    (void)sock;
    (void)lep;
    (void)tout;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_udp_unicast(_z_sys_net_socket_t *sock) {
    sock->_udp->stop();
    delete sock->_udp;
}

size_t _z_read_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    // Block until something to read
    // FIXME: provide some kind of timeout functionality
    ssize_t rb = 0;
    do {
        rb = sock._udp->parsePacket();
    } while (rb == 0);

    if (rb <= (ssize_t)len) {
        if (sock._udp->read(ptr, rb) != rb) {
            rb = 0;
        }
    }

    return rb;
}

size_t _z_read_exact_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_udp_unicast(sock, pos, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_udp_unicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len,
                           const _z_sys_net_endpoint_t rep) {
    sock._udp->beginPacket(*rep._iptcp._addr, rep._iptcp._port);
    sock._udp->write(ptr, len);
    sock._udp->endPacket();

    return len;
}
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1
int8_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                             uint32_t tout, const char *iface) {
    int8_t ret = _Z_RES_OK;
    (void)(rep);

    sock->_udp = new WiFiUDP();
    if (!sock->_udp->begin(55555)) {  // FIXME: make port to be random
        ret = _Z_ERR_GENERIC;
    }

    // Multicast messages are not self-consumed.
    // However, the clean-up process requires both rep and lep to be initialized.
    lep->_iptcp._addr = new IPAddress();

    if (ret != _Z_RES_OK) {
        delete sock->_udp;
    }

    return ret;
}

int8_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout,
                               const char *iface, const char *join) {
    (void)join;
    int8_t ret = _Z_RES_OK;

    sock->_udp = new WiFiUDP();
    if (!sock->_udp->beginMulticast(*rep._iptcp._addr, rep._iptcp._port)) {
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        delete sock->_udp;
    }

    return ret;
}

void _z_close_udp_multicast(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend,
                            const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep) {
    _ZP_UNUSED(rep);
    _ZP_UNUSED(lep);

    sockrecv->_udp->stop();
    delete sockrecv->_udp;

    socksend->_udp->stop();
    delete socksend->_udp;
}

size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                             _z_slice_t *addr) {
    // Block until something to read
    // FIXME: provide some kind of timeout functionality
    ssize_t rb = 0;
    do {
        rb = sock._udp->parsePacket();
    } while (rb == 0);

    if (rb <= (ssize_t)len) {
        if (sock._udp->read(ptr, rb) == rb) {
            if (addr != NULL) {  // If addr is not NULL, it means that the raddr was requested by the upper-layers
                IPAddress rip = sock._udp->remoteIP();
                uint16_t rport = sock._udp->remotePort();

                *addr = _z_slice_make(strlen((const char *)&rip[0]) + strlen((const char *)&rip[1]) +
                                      strlen((const char *)&rip[2]) + strlen((const char *)&rip[3]) + sizeof(uint16_t));
                uint8_t offset = 0;
                for (uint8_t i = 0; i < (uint8_t)4; i++) {
                    (void)memcpy(const_cast<uint8_t *>(addr->start + offset), &rip[i], strlen((const char *)&rip[i]));
                    offset = offset + (uint8_t)strlen((const char *)&rip[i]);
                }
                (void)memcpy(const_cast<uint8_t *>(addr->start + offset), &rport, sizeof(uint16_t));
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
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
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
#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on OpenCR port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_SERIAL == 1
#error "Serial not supported yet on OpenCR port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on OpenCR port of Zenoh-Pico"
#endif
}
