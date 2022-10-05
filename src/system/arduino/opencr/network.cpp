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

#if Z_LINK_TCP == 1

/*------------------ UDP sockets ------------------*/
_z_sys_net_endpoint_t _z_create_endpoint_tcp(const char *s_addr, const char *s_port) {
    _z_sys_net_endpoint_t ep;

    // Parse and check the validity of the IP address
    ep._iptcp._addr = new IPAddress();
    if (!ep._iptcp._addr->fromString(s_addr)) {
        goto ERR;
    }

    // Parse and check the validity of the port
    ep._iptcp._port = strtoul(s_port, NULL, 10);
    if ((ep._iptcp._port < (uint32_t)1) ||
        (ep._iptcp._port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        goto ERR;
    }

    ep._err = true;
    return ep;

ERR:
    delete ep._iptcp._addr;
    ep._err = false;
    return ep;
}

void _z_free_endpoint_tcp(_z_sys_net_endpoint_t ep) { delete ep._iptcp._addr; }

_z_sys_net_socket_t _z_open_tcp(_z_sys_net_endpoint_t rep, uint32_t tout) {
    _z_sys_net_socket_t sock;

    sock._tcp = new WiFiClient();
    if (!sock._tcp->connect(*rep._iptcp._addr, rep._iptcp._port)) {
        goto ERR;
    }

    sock._err = false;
    return sock;

ERR:
    delete sock._tcp;
    sock._err = true;
    return sock;
}

_z_sys_net_socket_t _z_listen_tcp(_z_sys_net_endpoint_t lep) {
    _z_sys_net_socket_t sock;
    (void)lep;

    // @TODO: To be implemented

    sock._err = true;
    return sock;
}

void _z_close_tcp(_z_sys_net_socket_t sock) {
    if (sock._err == true) {
        return;
    }

    sock._tcp->stop();
    delete sock._tcp;
}

size_t _z_read_tcp(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    if (!sock._tcp->available()) {
        return 0;
    }

    return sock._tcp->read(ptr, len);
}

size_t _z_read_exact_tcp(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_tcp(sock, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = _z_ptr_u8_offset(ptr, len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_tcp(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    sock._tcp->write(ptr, len);
    return len;
}

#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1

/*------------------ UDP sockets ------------------*/
_z_sys_net_endpoint_t _z_create_endpoint_udp(const char *s_addr, const char *s_port) {
    _z_sys_net_endpoint_t ep;

    // Parse and check the validity of the IP address
    ep._iptcp._addr = new IPAddress();
    if (!ep._iptcp._addr->fromString(s_addr)) {
        goto ERR;
    }

    // Parse and check the validity of the port
    ep._iptcp._port = strtoul(s_port, NULL, 10);
    if ((ep._iptcp._port < (uint32_t)1) ||
        (ep._iptcp._port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        goto ERR;
    }

    ep._err = true;
    return ep;

ERR:
    delete ep._iptcp._addr;
    ep._err = false;
    return ep;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t ep) { delete ep._iptcp._addr; }
#endif

#if Z_LINK_UDP_UNICAST == 1

_z_sys_net_socket_t _z_open_udp_unicast(_z_sys_net_endpoint_t rep, uint32_t tout) {
    _z_sys_net_socket_t sock;
    (void)(rep);

    // FIXME: make it random
    sock._udp = new WiFiUDP();
    if (!sock._udp->begin(7447)) {
        goto ERR;
    }

    sock._err = false;
    return sock;

ERR:
    delete sock._udp;
    sock._err = true;
    return sock;
}

_z_sys_net_socket_t _z_listen_udp_unicast(_z_sys_net_endpoint_t lep, uint32_t tout) {
    _z_sys_net_socket_t sock;
    (void)lep;
    (void)tout;

    // @TODO: To be implemented

    sock._err = false;
    return sock;
}

void _z_close_udp_unicast(_z_sys_net_socket_t sock) {
    if (sock._err == true) {
        return;
    }

    sock._udp->stop();
    delete sock._udp;
}

size_t _z_read_udp_unicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    // Block until something to read
    // FIXME; provide blocking and non-blocking functions
    size_t psize = 0;
    do {
        psize = sock._udp->parsePacket();
    } while (psize < (size_t)1);

    if (psize > len) {
        return 0;
    }

    if (sock._udp->read(ptr, psize) != psize) {
        return 0;
    }

    return psize;
}

size_t _z_read_exact_udp_unicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_udp_unicast(sock, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = _z_ptr_u8_offset(ptr, len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_udp_unicast(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len, _z_sys_net_endpoint_t rep) {
    sock._udp->beginPacket(*rep._iptcp._addr, rep._iptcp._port);
    sock._udp->write(ptr, len);
    sock._udp->endPacket();

    return len;
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
_z_sys_net_socket_t _z_open_udp_multicast(_z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep, uint32_t tout,
                                          const char *iface) {
    _z_sys_net_socket_t sock;
    (void)(rep);
    (void)(lep);  // Multicast messages are not self-consumed, so no need to save the local address

    // FIXME: make it random
    sock._udp = new WiFiUDP();
    if (sock._udp->begin(55555) == 0) {
        goto ERR;
    }

    sock._err = false;
    return sock;

ERR:
    delete sock._udp;
    sock._err = true;
    return sock;
}

_z_sys_net_socket_t _z_listen_udp_multicast(_z_sys_net_endpoint_t rep, uint32_t tout, const char *iface) {
    _z_sys_net_socket_t sock;

    sock._udp = new WiFiUDP();
    if (sock._udp->beginMulticast(*rep._iptcp._addr, rep._iptcp._port) == 0) {
        goto ERR;
    }

    sock._err = false;
    return sock;

ERR:
    delete sock._udp;
    sock._err = true;
    return sock;
}

void _z_close_udp_multicast(_z_sys_net_socket_t sockrecv, _z_sys_net_socket_t socksend, _z_sys_net_endpoint_t rep) {
    // Both sockrecv and socksend must be compared to err,
    //  because we dont know if the close is trigger by a normal close
    //  or some of the sockets failed during the opening/listening procedure.
    if (sockrecv._err != true) {
        sockrecv._udp->stop();
        delete sockrecv._udp;
    }

    if (socksend._err != true) {
        socksend._udp->stop();
        delete socksend._udp;
    }
}

size_t _z_read_udp_multicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len, _z_sys_net_endpoint_t lep,
                             _z_bytes_t *addr) {
    // Block until something to read
    // FIXME; provide blocking and non-blocking functions
    size_t psize = 0;
    do {
        psize = sock._udp->parsePacket();
    } while (psize == (size_t)0);

    if (psize > len) {
        return 0;
    }

    if (sock._udp->read(ptr, psize) != psize) {
        return 0;
    }

    // If addr is not NULL, it means that the raddr was requested by the upper-layers
    if (addr != NULL) {
        IPAddress rip = sock._udp->remoteIP();
        uint16_t rport = sock._udp->remotePort();

        *addr = _z_bytes_make(strlen((const char *)&rip[0]) + strlen((const char *)&rip[1]) +
                              strlen((const char *)&rip[2]) + strlen((const char *)&rip[3]) + sizeof(uint16_t));
        uint8_t offset = 0;
        for (uint8_t i = 0; i < (uint8_t)4; i++) {
            (void)memcpy(const_cast<uint8_t *>(addr->start + offset), &rip[i], strlen((const char *)&rip[i]));
            offset += (uint8_t)strlen((const char *)&rip[i]);
        }
        (void)memcpy(const_cast<uint8_t *>(addr->start + offset), &rport, sizeof(uint16_t));
    }

    return psize;
}

size_t _z_read_exact_udp_multicast(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len, _z_sys_net_endpoint_t lep,
                                   _z_bytes_t *addr) {
    size_t n = len;

    do {
        size_t rb = _z_read_udp_multicast(sock, ptr, n, lep, addr);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = _z_ptr_u8_offset(ptr, len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_udp_multicast(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len, _z_sys_net_endpoint_t rep) {
    sock._udp->beginPacket(*rep._iptcp._addr, rep._iptcp._port);
    sock._udp->write(ptr, len);
    sock._udp->endPacket();

    return len;
}
#endif

#if Z_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on OpenCR port of Zenoh-Pico"
#endif

#if Z_LINK_SERIAL == 1
#error "Serial not supported yet on OpenCR port of Zenoh-Pico"
#endif
}
