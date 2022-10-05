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
#include <stddef.h>
#include <string.h>

extern "C" {
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_LINK_TCP == 1

typedef struct {
    IPAddress _ipaddr;
    uint16_t _port;
} __z_tcp_addr_t;

/*------------------ UDP sockets ------------------*/
void *_z_create_endpoint_tcp(const char *s_addr, const char *s_port) {
    __z_tcp_addr_t *addr = new __z_tcp_addr_t();
    uint32_t port = 0;

    // Parse and check the validity of the IP address
    if (!addr->_ipaddr.fromString(s_addr)) {
        goto ERR;
    }

    // Parse and check the validity of the port
    port = strtoul(s_port, NULL, 10);
    if ((port < (uint32_t)1) || (port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        goto ERR;
    }
    addr->_port = port;

    return addr;

ERR:
    delete addr;
    return NULL;
}

void _z_free_endpoint_tcp(void *addr_arg) {
    __z_tcp_addr_t *addr = static_cast<__z_tcp_addr_t *>(addr_arg);

    delete addr;
}

void *_z_open_tcp(void *raddr_arg, uint32_t tout) {
    __z_tcp_addr_t *raddr = static_cast<__z_tcp_addr_t *>(raddr_arg);

    WiFiClient *sock = new WiFiClient();
    if (!sock->connect(raddr->_ipaddr, raddr->_port)) {
        return NULL;
    }

    return sock;
}

void *_z_listen_tcp(void *laddr_arg, uint32_t tout) {
    __z_tcp_addr_t *laddr = static_cast<__z_tcp_addr_t *>(laddr_arg);
    (void)(laddr);

    // @TODO: To be implemented

    return NULL;
}

void _z_close_tcp(void *sock_arg) {
    WiFiClient *sock = static_cast<WiFiClient *>(sock_arg);
    if (sock == NULL) {
        return;
    }

    sock->stop();
    delete sock;
}

size_t _z_read_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    WiFiClient *sock = static_cast<WiFiClient *>(sock_arg);

    if (!sock->available()) {
        return 0;
    }

    return sock->read(ptr, len);
}

size_t _z_read_exact_tcp(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_tcp(sock_arg, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = _z_ptr_u8_offset(ptr, len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_tcp(void *sock_arg, const uint8_t *ptr, size_t len) {
    WiFiClient *sock = static_cast<WiFiClient *>(sock_arg);

    sock->write(ptr, len);

    return len;
}

#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1

typedef struct {
    IPAddress _ipaddr;
    uint16_t _port;
} __z_udp_addr_t;

/*------------------ UDP sockets ------------------*/
void *_z_create_endpoint_udp(const char *s_addr, const char *s_port) {
    __z_udp_addr_t *addr = new __z_udp_addr_t();
    uint32_t port = 0;

    // Parse and check the validity of the IP address
    if (!addr->_ipaddr.fromString(s_addr)) {
        goto ERR;
    }

    // Parse and check the validity of the port
    port = strtoul(s_port, NULL, 10);
    if ((port < (uint32_t)1) || (port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
        goto ERR;
    }
    addr->_port = port;

    return addr;

ERR:
    delete addr;
    return NULL;
}

void _z_free_endpoint_udp(void *addr_arg) {
    __z_udp_addr_t *addr = static_cast<__z_udp_addr_t *>(addr_arg);

    delete addr;
}
#endif

#if Z_LINK_UDP_UNICAST == 1

void *_z_open_udp_unicast(void *raddr_arg, uint32_t tout) {
    __z_udp_addr_t *raddr = static_cast<__z_udp_addr_t *>(raddr_arg);
    (void)(raddr);

    // FIXME: make it random
    WiFiUDP *sock = new WiFiUDP();
    if (!sock->begin(7447)) {
        return NULL;
    }

    return sock;
}

void *_z_listen_udp_unicast(void *laddr_arg, uint32_t tout) {
    __z_udp_addr_t *laddr = static_cast<__z_udp_addr_t *>(laddr_arg);
    (void)(laddr);

    // @TODO: To be implemented

    return NULL;
}

void _z_close_udp_unicast(void *sock_arg) {
    WiFiUDP *sock = static_cast<WiFiUDP *>(sock_arg);
    if (sock == NULL) {
        return;
    }

    sock->stop();
    delete sock;
}

size_t _z_read_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    WiFiUDP *sock = static_cast<WiFiUDP *>(sock_arg);

    // Block until something to read
    // FIXME; provide blocking and non-blocking functions
    size_t psize = 0;
    do {
        psize = sock->parsePacket();
    } while (psize < (size_t)1);

    if (psize > len) {
        return 0;
    }

    if (sock->read(ptr, psize) != psize) {
        return 0;
    }

    return psize;
}

size_t _z_read_exact_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_udp_unicast(sock_arg, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = _z_ptr_u8_offset(ptr, len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_udp_unicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg) {
    WiFiUDP *sock = static_cast<WiFiUDP *>(sock_arg);
    __z_udp_addr_t *raddr = static_cast<__z_udp_addr_t *>(raddr_arg);

    sock->beginPacket(raddr->_ipaddr, 7447);
    sock->write(ptr, len);
    sock->endPacket();

    return len;
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
void *_z_open_udp_multicast(void *raddr_arg, void **laddr_arg, uint32_t tout, const char *iface) {
    __z_udp_addr_t *raddr = static_cast<__z_udp_addr_t *>(raddr_arg);
    (void)(raddr);
    // __z_udp_addr_t *laddr = NULL;  // Multicast messages are not self-consumed, so no need to save the local address

    // FIXME: make it random
    WiFiUDP *sock = new WiFiUDP();
    if (sock->begin(55555) == 0) {
        return NULL;
    }

    return sock;
}

void *_z_listen_udp_multicast(void *laddr_arg, uint32_t tout, const char *iface) {
    __z_udp_addr_t *laddr = static_cast<__z_udp_addr_t *>(laddr_arg);

    WiFiUDP *sock = new WiFiUDP();
    if (sock->beginMulticast(laddr->_ipaddr, laddr->_port) == 0) {
        return NULL;
    }

    return sock;
}

void _z_close_udp_multicast(void *sockrecv_arg, void *socksend_arg, void *arg) {
    WiFiUDP *sockrecv = static_cast<WiFiUDP *>(sockrecv_arg);
    WiFiUDP *socksend = static_cast<WiFiUDP *>(socksend_arg);

    // Both sockrecv and socksend must be compared to NULL,
    //  because we dont know if the close is trigger by a normal close
    //  or some of the sockets failed during the opening/listening procedure.
    if (sockrecv != NULL) {
        sockrecv->stop();
        delete sockrecv;
    }

    if (socksend != NULL) {
        socksend->stop();
        delete socksend;
    }
}

size_t _z_read_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *laddr_arg, _z_bytes_t *addr) {
    WiFiUDP *sock = static_cast<WiFiUDP *>(sock_arg);

    // Block until something to read
    // FIXME; provide blocking and non-blocking functions
    size_t psize = 0;
    do {
        psize = sock->parsePacket();
    } while (psize == (size_t)0);

    if (psize > len) {
        return 0;
    }

    if (sock->read(ptr, psize) != psize) {
        return 0;
    }

    // If addr is not NULL, it means that the raddr was requested by the upper-layers
    if (addr != NULL) {
        IPAddress rip = sock->remoteIP();
        uint16_t rport = sock->remotePort();

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

size_t _z_read_exact_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *laddr_arg, _z_bytes_t *addr) {
    size_t n = len;

    do {
        size_t rb = _z_read_udp_multicast(sock_arg, ptr, n, laddr_arg, addr);
        if (rb == SIZE_MAX) {
            return rb;
        }

        n -= rb;
        ptr = _z_ptr_u8_offset(ptr, len - n);
    } while (n > (size_t)0);

    return len;
}

size_t _z_send_udp_multicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg) {
    WiFiUDP *sock = static_cast<WiFiUDP *>(sock_arg);
    __z_udp_addr_t *raddr = static_cast<__z_udp_addr_t *>(raddr_arg);

    sock->beginPacket(raddr->_ipaddr, 7447);
    sock->write(ptr, len);
    sock->endPacket();

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
