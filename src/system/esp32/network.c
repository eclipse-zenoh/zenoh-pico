/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <netdb.h>

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Endpoint ------------------*/
void *_zn_create_endpoint_tcp(const z_str_t s_addr, const z_str_t port)
{
    struct addrinfo hints;
    struct addrinfo *addr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC; // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_addr, port, &hints, &addr) < 0)
        return NULL;

    return addr;
}

void *_zn_create_endpoint_udp(const z_str_t s_addr, const z_str_t port)
{
    struct addrinfo hints;
    struct addrinfo *addr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC; // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(s_addr, port, &hints, &addr) < 0)
        return NULL;

    return addr;
}

void _zn_free_endpoint_tcp(void *arg)
{
    struct addrinfo *self = (struct addrinfo *)arg;

    freeaddrinfo(self);
}

void _zn_free_endpoint_udp(void *arg)
{
    struct addrinfo *self = (struct addrinfo *)arg;

    freeaddrinfo(self);
}

/*------------------ TCP sockets ------------------*/
int _zn_open_tcp(void *arg)
{
    struct addrinfo *raddr = (struct addrinfo *)arg;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0)
        goto _ZN_OPEN_TCP_ERROR_1;

    int flags = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)
        goto _ZN_OPEN_TCP_ERROR_2;

#if LWIP_SO_LINGER == 1
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = ZN_TRANSPORT_LEASE / 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)
        goto _ZN_OPEN_TCP_ERROR_2;
#endif

    struct addrinfo *it = NULL;
    for (it = raddr; it != NULL; it = it->ai_next)
    {
        if (connect(sock, it->ai_addr, it->ai_addrlen) < 0)
        {
            if (it->ai_next == NULL)
                goto _ZN_OPEN_TCP_ERROR_2;
        }
        else
            break;
    }

    return sock;

_ZN_OPEN_TCP_ERROR_2:
    close(sock);

_ZN_OPEN_TCP_ERROR_1:
    return -1;
}

int _zn_listen_tcp(void *arg)
{
    struct addrinfo *laddr = (struct addrinfo *)arg;

    // TODO: to be implemented

    return -1;
}

void _zn_close_tcp(int sock)
{
    shutdown(sock, SHUT_RDWR);
    close(sock);
}

size_t _zn_read_tcp(int sock, uint8_t *ptr, size_t len)
{
    return recv(sock, ptr, len, 0);
}

size_t _zn_read_exact_tcp(int sock, uint8_t *ptr, size_t len)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _zn_read_tcp(sock, ptr, n);
        if (rb < 0)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _zn_send_tcp(int sock, const uint8_t *ptr, size_t len)
{
    return send(sock, ptr, len, 0);
}

/*------------------ UDP sockets ------------------*/
int _zn_open_udp_unicast(void *arg, const clock_t tout)
{
    struct addrinfo *raddr = (struct addrinfo *)arg;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0)
        goto _ZN_OPEN_UDP_UNICAST_ERROR_1;

    return sock;

_ZN_OPEN_UDP_UNICAST_ERROR_1:
    return -1;
}

int _zn_listen_udp_unicast(void *arg, const clock_t tout)
{
    struct addrinfo *laddr = (struct addrinfo *)arg;

    // TODO: To be implemented

    return -1;
}

void _zn_close_udp_unicast(int sock)
{
    close(sock);
}

size_t _zn_read_udp_unicast(int sock, uint8_t *ptr, size_t len)
{
    struct sockaddr_storage raddr;
    unsigned int addrlen = sizeof(struct sockaddr_storage);

    size_t rb = recvfrom(sock, ptr, len, 0,
                         (struct sockaddr *)&raddr, &addrlen);

    return rb;
}

size_t _zn_read_exact_udp_unicast(int sock, uint8_t *ptr, size_t len)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _zn_read_udp_unicast(sock, ptr, n);
        if (rb < 0)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _zn_send_udp_unicast(int sock, const uint8_t *ptr, size_t len, void *arg)
{
    struct addrinfo *raddr = (struct addrinfo *)arg;

    return sendto(sock, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}

int _zn_open_udp_multicast(void *arg_1, void **arg_2, const clock_t tout, const z_str_t iface)
{
    struct addrinfo *raddr = (struct addrinfo *)arg_1;
    struct addrinfo *laddr = NULL;
    unsigned int addrlen = 0;

    struct sockaddr *lsockaddr = NULL;
    if (raddr->ai_family == AF_INET)
    {
        lsockaddr = (struct sockaddr *)malloc(sizeof(struct sockaddr_in));
        memset(lsockaddr, 0, sizeof(struct sockaddr_in));
        addrlen = sizeof(struct sockaddr_in);

        struct sockaddr_in *c_laddr = (struct sockaddr_in *)lsockaddr;
        c_laddr->sin_family = AF_INET;
        c_laddr->sin_addr.s_addr = INADDR_ANY;
        c_laddr->sin_port = htons(INADDR_ANY);
    }
    else if (raddr->ai_family == AF_INET6)
    {
        lsockaddr = (struct sockaddr *)malloc(sizeof(struct sockaddr_in6));
        memset(lsockaddr, 0, sizeof(struct sockaddr_in6));
        addrlen = sizeof(struct sockaddr_in6);

        struct sockaddr_in6 *c_laddr = (struct sockaddr_in6 *)lsockaddr;
        c_laddr->sin6_family = AF_INET6;
        c_laddr->sin6_addr = in6addr_any;
        c_laddr->sin6_port = htons(INADDR_ANY);
        //        c_laddr->sin6_scope_id; // Not needed to be defined
    }
    else
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_1;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0)
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_1;

    if (bind(sock, lsockaddr, addrlen) < 0)
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_2;

    if (getsockname(sock, lsockaddr, &addrlen) < -1)
        goto _ZN_OPEN_UDP_MULTICAST_ERROR_2;

    if (lsockaddr->sa_family == AF_INET)
    {
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &((struct sockaddr_in *)lsockaddr)->sin_addr, sizeof(struct in_addr)) < 0)
            goto _ZN_OPEN_UDP_MULTICAST_ERROR_2;
    }
    else if (lsockaddr->sa_family == AF_INET6)
    {
        int ifindex = 0;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0)
            goto _ZN_OPEN_UDP_MULTICAST_ERROR_2;
    }

    // Create laddr endpoint
    laddr = (struct addrinfo *)malloc(sizeof(struct addrinfo));
    laddr->ai_flags = 0;
    laddr->ai_family = raddr->ai_family;
    laddr->ai_socktype = raddr->ai_socktype;
    laddr->ai_protocol = raddr->ai_protocol;
    laddr->ai_addrlen = addrlen;
    laddr->ai_addr = lsockaddr;
    laddr->ai_canonname = NULL;
    laddr->ai_next = NULL;
    *arg_2 = laddr;

    return sock;

_ZN_OPEN_UDP_MULTICAST_ERROR_2:
    close(sock);

_ZN_OPEN_UDP_MULTICAST_ERROR_1:
    return -1;
}

int _zn_listen_udp_multicast(void *arg, const clock_t tout, const z_str_t iface)
{
    struct addrinfo *raddr = (struct addrinfo *)arg;
    struct sockaddr *laddr = NULL;
    unsigned int addrlen = 0;

    if (raddr->ai_family == AF_INET)
    {
        laddr = (struct sockaddr *)malloc(sizeof(struct sockaddr_in));
        memset(laddr, 0, sizeof(struct sockaddr_in));
        addrlen = sizeof(struct sockaddr_in);

        struct sockaddr_in *c_laddr = (struct sockaddr_in *)laddr;
        c_laddr->sin_family = AF_INET;
        c_laddr->sin_addr.s_addr = INADDR_ANY;
        c_laddr->sin_port = ((struct sockaddr_in *)raddr->ai_addr)->sin_port;
    }
    else if (raddr->ai_family == AF_INET6)
    {
        laddr = (struct sockaddr *)malloc(sizeof(struct sockaddr_in6));
        memset(laddr, 0, sizeof(struct sockaddr_in6));
        addrlen = sizeof(struct sockaddr_in6);

        struct sockaddr_in6 *c_laddr = (struct sockaddr_in6 *)laddr;
        c_laddr->sin6_family = AF_INET6;
        c_laddr->sin6_addr = in6addr_any;
        c_laddr->sin6_port = htons(INADDR_ANY);
        c_laddr->sin6_port = ((struct sockaddr_in6 *)raddr->ai_addr)->sin6_port;
        //        c_laddr->sin6_scope_id; // Not needed to be defined
    }
    else
        goto _ZN_LISTEN_UDP_MULTICAST_ERROR_1;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0)
        goto _ZN_LISTEN_UDP_MULTICAST_ERROR_1;

    int optflag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (z_str_t)&optflag, sizeof(optflag)) < 0)
        goto _ZN_LISTEN_UDP_MULTICAST_ERROR_2;

    if (bind(sock, laddr, addrlen) < 0)
        goto _ZN_LISTEN_UDP_MULTICAST_ERROR_2;

    // Join the multicast group
    if (raddr->ai_family == AF_INET)
    {
        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)raddr->ai_addr)->sin_addr.s_addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
            goto _ZN_LISTEN_UDP_MULTICAST_ERROR_2;
    }
    else if (raddr->ai_family == AF_INET6)
    {
        struct ip6_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        memcpy(&mreq.ipv6mr_multiaddr,
               &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr,
               sizeof(struct in6_addr));
        //        mreq.ipv6mr_interface = ifindex;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0)
            goto _ZN_LISTEN_UDP_MULTICAST_ERROR_2;
    }
    else
        goto _ZN_LISTEN_UDP_MULTICAST_ERROR_2;

    return sock;

_ZN_LISTEN_UDP_MULTICAST_ERROR_2:
    close(sock);

_ZN_LISTEN_UDP_MULTICAST_ERROR_1:
    return -1;
}

void _zn_close_udp_multicast(int sock_recv, int sock_send, void *arg)
{
    struct addrinfo *raddr = (struct addrinfo *)arg;

    if (raddr->ai_family == AF_INET)
    {
        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *)raddr->ai_addr)->sin_addr.s_addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        setsockopt(sock_recv, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    }
    else if (raddr->ai_family == AF_INET6)
    {
        // struct ipv6_mreq mreq;
        // memset(&mreq, 0, sizeof(mreq));
        // memcpy(&mreq.ipv6mr_multiaddr,
        //  &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr,
        //  sizeof(struct in6_addr));
        // // mreq.ipv6mr_interface = ifindex;
        // setsockopt(sock_recv, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq));
    }

    close(sock_recv);
    close(sock_send);
}

size_t _zn_read_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg)
{
    struct addrinfo *laddr = (struct addrinfo *)arg;
    struct sockaddr_storage raddr;
    unsigned int addrlen = sizeof(struct sockaddr_storage);

    size_t rb = 0;
    do
    {
        rb = recvfrom(sock, ptr, len, 0,
                      (struct sockaddr *)&raddr, &addrlen);

        if (laddr->ai_family == AF_INET)
        {
            struct sockaddr_in *a = ((struct sockaddr_in *)laddr->ai_addr);
            struct sockaddr_in *b = ((struct sockaddr_in *)&raddr);
            if (!(a->sin_port == b->sin_port && a->sin_addr.s_addr == b->sin_addr.s_addr))
                break;
        }
        else if (laddr->ai_family == AF_INET6)
        {
            struct sockaddr_in6 *a = ((struct sockaddr_in6 *)laddr->ai_addr);
            struct sockaddr_in6 *b = ((struct sockaddr_in6 *)&raddr);
            if (a->sin6_port != b->sin6_port || memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(struct in6_addr)) != 0)
                break;
        }
    } while (1);

    return rb;
}

size_t _zn_read_exact_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg)
{
    size_t n = len;
    size_t rb = 0;

    do
    {
        rb = _zn_read_udp_multicast(sock, ptr, n, arg);
        if (rb < 0)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

size_t _zn_send_udp_multicast(int sock, const uint8_t *ptr, size_t len, void *arg)
{
    struct addrinfo *raddr = (struct addrinfo *)arg;

    return sendto(sock, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}
