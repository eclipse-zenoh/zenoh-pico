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

#include <string.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <Arduino.h>

extern "C"
{
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/collections/string.h"

#if Z_LINK_TCP == 1

typedef struct
{
    IPAddress _ipaddr;
    uint16_t _port;
} __z_tcp_addr_t;

/*------------------ UDP sockets ------------------*/
void *_z_create_endpoint_tcp(const char *s_addr, const char *port)
{
    __z_tcp_addr_t *addr = (__z_tcp_addr_t *)z_malloc(sizeof(__z_tcp_addr_t));
    addr->_ipaddr.fromString(s_addr);
    addr->_port = atoi(port);

    return addr;
}

void _z_free_endpoint_tcp(void *addr_arg)
{
    __z_tcp_addr_t *addr = (__z_tcp_addr_t *)addr_arg;
    z_free(addr);
}

void *_z_open_tcp(void *raddr_arg, uint32_t tout)
{
    __z_tcp_addr_t *raddr = (__z_tcp_addr_t *)raddr_arg;

    WiFiClient *sock = new WiFiClient();
    if (!sock->connect(raddr->_ipaddr, raddr->_port))
        return NULL;

    return sock;
}

void * _z_listen_tcp(void *laddr_arg, uint32_t tout)
{
    __z_tcp_addr_t *laddr = (__z_tcp_addr_t *)laddr_arg;

    // @TODO: To be implemented

    return NULL;
}

void _z_close_tcp(void *sock_arg)
{
    WiFiClient *sock = (WiFiClient *)sock_arg;
    if (sock == NULL)
        return;

    sock->stop();
    delete sock;
}

size_t _z_read_tcp(void *sock_arg, uint8_t *ptr, size_t len)
{
    WiFiClient *sock = (WiFiClient *)sock_arg;

    if (!sock->available())
        return 0;

    return sock->read(ptr, len);
}

size_t _z_read_exact_tcp(void *sock_arg, uint8_t *ptr, size_t len)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _z_read_tcp(sock_arg, ptr, n);
        if (rb == SIZE_MAX)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_tcp(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg)
{
    WiFiClient *sock = (WiFiClient *)sock_arg;
    __z_tcp_addr_t *raddr = (__z_tcp_addr_t *)raddr_arg;

    sock->write(ptr, len);

    return len;
}

#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1

typedef struct
{
    IPAddress _ipaddr;
    uint16_t _port;
} __z_udp_addr_t;

/*------------------ UDP sockets ------------------*/
void *_z_create_endpoint_udp(const char *s_addr, const char *port)
{
    __z_udp_addr_t *addr = (__z_udp_addr_t *)z_malloc(sizeof(__z_udp_addr_t));
    addr->_ipaddr.fromString(s_addr);
    addr->_port = atoi(port);

    return addr;
}

void _z_free_endpoint_udp(void *addr_arg)
{
    __z_udp_addr_t *addr = (__z_udp_addr_t *)addr_arg;
    z_free(addr);
}
#endif

#if Z_LINK_UDP_UNICAST == 1

void *_z_open_udp_unicast(void *raddr_arg, uint32_t tout)
{
    __z_udp_addr_t *raddr = (__z_udp_addr_t *)raddr_arg;

    WiFiUDP *sock = new WiFiUDP();
    if (!sock->begin(7447)) // FIXME: make it random
        return NULL;

    return sock;
}

void * _z_listen_udp_unicast(void *laddr_arg, uint32_t tout)
{
    __z_udp_addr_t *laddr = (__z_udp_addr_t *)laddr_arg;

    // @TODO: To be implemented

    return NULL;
}

void _z_close_udp_unicast(void *sock_arg)
{
    WiFiUDP *sock = (WiFiUDP *)sock_arg;
    if (sock == NULL)
        return;

    sock->stop();
    delete sock;
}

size_t _z_read_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len)
{
    WiFiUDP *sock = (WiFiUDP *)sock_arg;

    // Block until something to read
    // FIXME; provide blocking and non-blocking functions
    int psize = 0;
    do
    {
        psize = sock->parsePacket();
    } while (psize < 1);

    if (psize > len)
        return 0;

    if (sock->read(ptr, psize) != psize)
        return 0;

    return psize;
}

size_t _z_read_exact_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _z_read_udp_unicast(sock_arg, ptr, n);
        if (rb == SIZE_MAX)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_udp_unicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg)
{
    WiFiUDP *sock = (WiFiUDP *)sock_arg;
    __z_udp_addr_t *raddr = (__z_udp_addr_t *)raddr_arg;

    sock->beginPacket(raddr->_ipaddr, 7447);
    sock->write(ptr, len);
    sock->endPacket();

    return len;
}
#endif

#if Z_LINK_UDP_MULTICAST == 1
void *_z_open_udp_multicast(void *raddr_arg, void **laddr_arg, uint32_t tout, const char *iface)
{
    __z_udp_addr_t *raddr = (__z_udp_addr_t *)raddr_arg;
    __z_udp_addr_t *laddr = NULL; // Multicast messages are not self-consumed,
                                   // so no need to save the local address

    WiFiUDP *sock = new WiFiUDP();
    if (!sock->begin(55555)) // FIXME: make it random
        return NULL;

    return sock;
}

void *_z_listen_udp_multicast(void *raddr_arg, uint32_t tout, const char *iface)
{
    __z_udp_addr_t *raddr = (__z_udp_addr_t *)raddr_arg;

    WiFiUDP *sock = new WiFiUDP();
    if (!sock->beginMulticast(raddr->_ipaddr, raddr->_port))
        return NULL;

    return sock;
}

void _z_close_udp_multicast(void *sockrecv_arg, void *socksend_arg, void *arg)
{
    WiFiUDP *sockrecv = (WiFiUDP *)sockrecv_arg;
    WiFiUDP *socksend = (WiFiUDP *)socksend_arg;

    // Both sockrecv and socksend must be compared to NULL,
    //  because we dont know if the close is trigger by a normal close
    //  or some of the sockets failed during the opening/listening procedure.
    if (sockrecv != NULL)
    {
        sockrecv->stop();
        delete sockrecv;
    }

    if (socksend != NULL)
    {
        socksend->stop();
        delete socksend;
    }
}

size_t _z_read_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *laddr_arg, _z_bytes_t *addr)
{
    WiFiUDP *sock = (WiFiUDP *)sock_arg;

    // Block until something to read
    // FIXME; provide blocking and non-blocking functions
    int psize = 0;
    do
    {
        psize = sock->parsePacket();
    } while (psize < 1);

    if (psize > len)
        return 0;

    if (sock->read(ptr, psize) != psize)
        return 0;

    // If addr is not NULL, it means that the raddr was requested by the upper-layers
    if (addr != NULL)
    {
        IPAddress rip = sock->remoteIP();
        uint16_t rport = sock->remotePort();

        *addr = _z_bytes_make(strlen((const char*)&rip[0]) + strlen((const char*)&rip[1]) + strlen((const char*)&rip[2]) + strlen((const char*)&rip[3]) + sizeof(uint16_t));
        int offset = 0;
        for (int i = 0; i < 4; i++)
        {
            memcpy((uint8_t *)addr->start + offset, &rip[i], strlen((const char*)&rip[i]));
            offset += strlen((const char*)&rip[i]);
        }
        memcpy((uint8_t *)addr->start + offset, &rport, sizeof(uint16_t));
    }

    return psize;
}

size_t _z_read_exact_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *laddr_arg, _z_bytes_t *addr)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _z_read_udp_multicast(sock_arg, ptr, n, laddr_arg, addr);
        if (rb == SIZE_MAX)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _z_send_udp_multicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg)
{
    WiFiUDP *sock = (WiFiUDP *)sock_arg;
    __z_udp_addr_t *raddr = (__z_udp_addr_t *)raddr_arg;

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