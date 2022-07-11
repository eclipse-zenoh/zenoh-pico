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

#include <mbed.h>

extern "C"
{
#include <netdb.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/collections/string.h"

#if ZN_LINK_TCP == 1
/*------------------ TCP sockets ------------------*/
void *_zn_create_endpoint_tcp(const z_str_t s_addr, const z_str_t port)
{
    SocketAddress *addr = new SocketAddress(s_addr, atoi(port));
    return addr;
}

void _zn_free_endpoint_tcp(void *addr_arg)
{
    SocketAddress *addr = (SocketAddress *)addr_arg;
    delete addr;
}

TCPSocket *_zn_open_tcp(void *raddr_arg, const clock_t tout)
{
    NetworkInterface *net = NetworkInterface::get_default_instance();
    SocketAddress *raddr = (SocketAddress *)raddr_arg;

    TCPSocket *sock = new TCPSocket();
    sock->set_timeout(tout * 1000);
    if (sock->open(net) < 0)
        goto _ZN_OPEN_TCP_UNICAST_ERROR_1;

    if (sock->connect(*raddr) < 0)
        goto _ZN_OPEN_TCP_UNICAST_ERROR_1;

    return sock;

_ZN_OPEN_TCP_UNICAST_ERROR_1:
    delete sock;
    return NULL;
}

TCPSocket *_zn_listen_tcp(void *raddr_arg)
{
    // @TODO: to be implemented

    return NULL;
}

void _zn_close_tcp(void *sock_arg)
{
    TCPSocket *sock = (TCPSocket *)sock_arg;
    sock->close();
    delete sock;
}

size_t _zn_read_tcp(void *sock_arg, uint8_t *ptr, size_t len)
{
    TCPSocket *sock = (TCPSocket *)sock_arg;
    nsapi_size_or_error_t rb = sock->recv(ptr, len);
    if (rb < 0)
        return SIZE_MAX;

    return rb;
}

size_t _zn_read_exact_tcp(void *sock_arg, uint8_t *ptr, size_t len)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _zn_read_tcp(sock_arg, ptr, n);
        if (rb == SIZE_MAX)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _zn_send_tcp(void *sock_arg, const uint8_t *ptr, size_t len)
{
    TCPSocket *sock = (TCPSocket *)sock_arg;
    return sock->send(ptr, len);
}
#endif

#if ZN_LINK_UDP_UNICAST == 1 || ZN_LINK_UDP_MULTICAST == 1
/*------------------ UDP sockets ------------------*/
void *_zn_create_endpoint_udp(const z_str_t s_addr, const z_str_t port)
{
    SocketAddress *addr = new SocketAddress(s_addr, atoi(port));
    return addr;
}

void _zn_free_endpoint_udp(void *addr_arg)
{
    SocketAddress *addr = (SocketAddress *)addr_arg;
    delete addr;
}
#endif

#if ZN_LINK_UDP_UNICAST == 1
UDPSocket *_zn_open_udp_unicast(void *raddr_arg, const clock_t tout)
{
    NetworkInterface *net = NetworkInterface::get_default_instance();

    UDPSocket *sock = new UDPSocket();
    sock->set_timeout(tout * 1000);
    if (sock->open(net) < 0)
        goto _ZN_OPEN_UDP_UNICAST_ERROR_1;

    return sock;

_ZN_OPEN_UDP_UNICAST_ERROR_1:
    delete sock;
    return NULL;
}

UDPSocket *_zn_listen_udp_unicast(void *raddr_arg, const clock_t tout)
{
    // @TODO: To be implemented

    return NULL;
}

void _zn_close_udp_unicast(void *sock_arg)
{
    UDPSocket *sock = (UDPSocket *)sock_arg;
    sock->close();
    delete sock;
}

size_t _zn_read_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len)
{
    UDPSocket *sock = (UDPSocket *)sock_arg;

    SocketAddress raddr;
    nsapi_size_or_error_t rb = sock->recvfrom(&raddr, ptr, len);
    if (rb < 0)
        return SIZE_MAX;

    return rb;
}

size_t _zn_read_exact_udp_unicast(void *sock_arg, uint8_t *ptr, size_t len)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _zn_read_udp_unicast(sock_arg, ptr, n);
        if (rb == SIZE_MAX)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _zn_send_udp_unicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg)
{
    UDPSocket *sock = (UDPSocket *)sock_arg;
    SocketAddress *raddr = (SocketAddress *)raddr_arg;

    int sb = sock->sendto(*raddr, ptr, len);
    return sb;
}
#endif

#if ZN_LINK_UDP_MULTICAST == 1
UDPSocket *_zn_open_udp_multicast(void *raddr_arg, void **laddr_arg, const clock_t tout, const z_str_t iface)
{
    *laddr_arg = NULL; // Multicast messages are not self-consumed,
                       // so no need to save the local address

    NetworkInterface *net = NetworkInterface::get_default_instance();

    UDPSocket *sock = new UDPSocket();
    sock->set_timeout(tout * 1000);
    if (sock->open(net) < 0)
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_1;

    return sock;

_ZN_OPEN_UDP_MULTICAST_ERROR_1:
    delete sock;
    return NULL;
}

UDPSocket *_zn_listen_udp_multicast(void *raddr_arg, const clock_t tout, const z_str_t iface)
{
    NetworkInterface *net = NetworkInterface::get_default_instance();
    SocketAddress *raddr = (SocketAddress *)raddr_arg;

    UDPSocket *sock = new UDPSocket();
    sock->set_timeout(tout * 1000);
    if (sock->open(net) < 0)
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_1;

    if (sock->bind(raddr->get_port()) < 0)
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_1;

    if (sock->join_multicast_group(*raddr))
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_1;

    return sock;

_ZN_OPEN_UDP_MULTICAST_ERROR_1:
    delete sock;
    return NULL;
}

void _zn_close_udp_multicast(void *sockrecv_arg, void *socksend_arg, void *raddr_arg)
{
    UDPSocket *sockrecv = (UDPSocket *)sockrecv_arg;
    UDPSocket *socksend = (UDPSocket *)socksend_arg;
    SocketAddress *raddr = (SocketAddress *)raddr_arg;

    sockrecv->leave_multicast_group(*raddr);

    sockrecv->close();
    socksend->close();

    delete sockrecv;
    delete socksend;
}

size_t _zn_read_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *laddr_arg, z_bytes_t *addr)
{
    UDPSocket *sock = (UDPSocket *)sock_arg;
    SocketAddress raddr;
    nsapi_size_or_error_t rb = 0;

    do
    {
        rb = sock->recvfrom(&raddr, ptr, len);
        if (rb < 0)
            return SIZE_MAX;

        if (raddr.get_ip_version() == NSAPI_IPv4)
        {
            *addr = _z_bytes_make(NSAPI_IPv4_BYTES + sizeof(uint16_t));
            memcpy((void *)addr->val, raddr.get_ip_bytes(), NSAPI_IPv4_BYTES);
            uint16_t port = raddr.get_port();
            memcpy((void *)(addr->val + NSAPI_IPv4_BYTES), &port, sizeof(uint16_t));
            break;
        }
        else if (raddr.get_ip_version() == NSAPI_IPv6)
        {
            *addr = _z_bytes_make(NSAPI_IPv6_BYTES + sizeof(uint16_t));
            memcpy((void *)addr->val, raddr.get_ip_bytes(), NSAPI_IPv6_BYTES);
            uint16_t port = raddr.get_port();
            memcpy((void *)(addr->val + NSAPI_IPv6_BYTES), &port, sizeof(uint16_t));
            break;
        }
    } while (1);

    return rb;
}

size_t _zn_read_exact_udp_multicast(void *sock_arg, uint8_t *ptr, size_t len, void *arg, z_bytes_t *addr)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _zn_read_udp_multicast(sock_arg, ptr, n, arg, addr);
        if (rb == SIZE_MAX)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _zn_send_udp_multicast(void *sock_arg, const uint8_t *ptr, size_t len, void *raddr_arg)
{
    UDPSocket *sock = (UDPSocket *)sock_arg;
    SocketAddress *raddr = (SocketAddress *)raddr_arg;

    int sb = sock->sendto(*raddr, ptr, len);
    return sb;
}
#endif

#if ZN_LINK_BLUETOOTH == 1
    #error "Bluetooth not supported yet on MBED port of Zenoh-Pico"
#endif

} // extern "C"