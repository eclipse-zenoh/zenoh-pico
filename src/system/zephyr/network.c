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
#include <sys/socket.h>

#include "zenoh-pico/system/common.h"
#include "zenoh-pico/utils/private/logging.h"

/*------------------ Endpoint ------------------*/
void* _zn_create_endpoint_tcp(const char *s_addr, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *addr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;       // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_addr, port, &hints, &addr) < 0)
        return NULL;

    return addr;
}

void* _zn_create_endpoint_udp(const char *s_addr, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *addr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;       // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(s_addr, port, &hints, &addr) < 0)
        return NULL;

    return addr;
}

void _zn_release_endpoint_tcp(void *arg)
{
    struct addrinfo *self = (struct addrinfo*)arg;
    freeaddrinfo(self);
}

void _zn_release_endpoint_udp(void *arg)
{
    struct addrinfo *self = (struct addrinfo*)arg;
    freeaddrinfo(self);
}

/*------------------ TCP sockets ------------------*/
int _zn_open_tcp(void *arg)
{
    struct addrinfo *raddr = (struct addrinfo*)arg;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0)
        return -1;

#if LWIP_SO_LINGER == 1
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = ZN_TRANSPORT_LEASE / 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)
    {
        close(sock);
        return -1;
    }
#endif

    struct addrinfo *it = NULL;
    for (it = raddr; it != NULL; it = it->ai_next)
    {
        if (connect(sock, it->ai_addr, it->ai_addrlen) < 0)
        {
            if (it->ai_next == NULL)
                return -1;
        }
        else
        {
            break;
        }
    }

    return sock;
}

int _zn_listen_tcp(void *arg)
{
    struct addrinfo *laddr = (struct addrinfo*)arg;

    // TODO: to be implemented

    return -1;
}

int _zn_close_tcp(int sock)
{
    return shutdown(sock, SHUT_RDWR);
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
    struct addrinfo *raddr = (struct addrinfo*)arg;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0)
        return -1;

    return sock;
}

int _zn_listen_udp_unicast(void *arg, const clock_t tout)
{
    struct addrinfo *laddr = (struct addrinfo*)arg;

    // TODO: to be implemented

    return -1;
}

int _zn_close_udp_unicast(int sock)
{
    return close(sock);
}

size_t _zn_read_udp_unicast(int sock, uint8_t *ptr, size_t len)
{
    return recv(sock, ptr, len, 0);
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
    struct addrinfo *raddr = (struct addrinfo*) arg;

    return sendto(sock, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}

int _zn_open_udp_multicast(void *arg_1, void **arg_2, const clock_t tout, const char *iface)
{
    struct addrinfo *raddr = (struct addrinfo *)arg_1;
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
        c_laddr->sin_port = htons(INADDR_ANY);
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
//        c_laddr->sin6_scope_id; // Not needed to be defined
    }
    else
        return -1;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0)
        return -1;

    if (bind(sock, laddr, addrlen) < 0)
        return -1;

    // Get the randomly assigned port used to discard loopback messages
    if (getsockname(sock, laddr, &addrlen) < 0)
       return -1;

    *arg_2 = laddr;
    return sock;
}

int _zn_listen_udp_multicast(void *arg, const clock_t tout, const char *iface)
{
    struct addrinfo *raddr = (struct addrinfo*)arg;
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
        return -1;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0)
        goto EXIT_MULTICAST_LISTEN_ERROR;

    int optflag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optflag, sizeof(optflag)) < 0)
        goto EXIT_MULTICAST_LISTEN_ERROR;

    if (bind(sock, laddr, addrlen) < 0)
        goto EXIT_MULTICAST_LISTEN_ERROR;

    // FIXME: iface passed into the locator is being ignored
    //        default if used instead
    struct net_if *ifa = NULL;
    ifa = net_if_get_default();
    if (!ifa)
        goto EXIT_MULTICAST_LISTEN_ERROR;

    // Join the multicast group
    if (raddr->ai_family == AF_INET)
    {
        struct net_if_mcast_addr *mcast = NULL;
        mcast = net_if_ipv4_maddr_add(ifa, &((struct sockaddr_in *)raddr->ai_addr)->sin_addr);
        if (!mcast)
            goto EXIT_MULTICAST_LISTEN_ERROR;
        net_if_ipv4_maddr_join(mcast);
    }
    else if(raddr->ai_family == AF_INET6)
    {
        struct net_if_mcast_addr *mcast = NULL;
        mcast = net_if_ipv6_maddr_add(ifa, &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr);
        if (!mcast)
            goto EXIT_MULTICAST_LISTEN_ERROR;
        net_if_ipv6_maddr_join(mcast);
    }
    else
        goto EXIT_MULTICAST_LISTEN_ERROR;

    return sock;

EXIT_MULTICAST_LISTEN_ERROR:
    close(sock);

    return -1;
}

int _zn_close_udp_multicast(int sock, void *arg)
{
    struct addrinfo *raddr = (struct addrinfo*)arg;

    // FIXME: iface passed into the locator is being ignored
    //        default if used instead
    struct net_if *ifa = NULL;
    ifa = net_if_get_default();
    if (!ifa)
        goto EXIT_MULTICAST_CLOSE;

    if (raddr->ai_family == AF_INET)
    {
        struct net_if_mcast_addr *mcast = NULL;
        mcast = net_if_ipv4_maddr_add(ifa, &((struct sockaddr_in *)raddr->ai_addr)->sin_addr);
        if (!mcast)
            goto EXIT_MULTICAST_CLOSE;

        net_if_ipv4_maddr_leave(mcast);
        net_if_ipv4_maddr_rm(ifa, &((struct sockaddr_in *)raddr->ai_addr)->sin_addr);
    }
    else if (raddr->ai_family == AF_INET6)
    {
        struct net_if_mcast_addr *mcast = NULL;
        mcast = net_if_ipv6_maddr_add(ifa, &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr);
        if (!mcast)
            goto EXIT_MULTICAST_CLOSE;

        net_if_ipv6_maddr_leave(mcast);
        net_if_ipv6_maddr_rm(ifa, &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr);
    }

EXIT_MULTICAST_CLOSE:
    return close(sock);
}

size_t _zn_read_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg)
{
    struct sockaddr *laddr = (struct sockaddr*)arg;
    struct sockaddr raddr;
    unsigned int addrlen = sizeof(struct sockaddr);

    size_t rb = 0;
    int is_loop = 1;
    do
    {
        rb = recvfrom(sock, ptr, len, 0,
                 (struct sockaddr *)&raddr, &addrlen);

        if (laddr->sa_family == AF_INET)
        {
            struct sockaddr_in *a = ((struct sockaddr_in *)laddr);
            struct sockaddr_in *b = ((struct sockaddr_in *)&raddr);
            if (!(a->sin_port == b->sin_port && a->sin_addr.s_addr == b->sin_addr.s_addr))
                is_loop = 0;
        }
        else if (laddr->sa_family == AF_INET6)
        {
            struct sockaddr_in6 *a = ((struct sockaddr_in6 *)laddr);
            struct sockaddr_in6 *b = ((struct sockaddr_in6 *)&raddr);
            if (!(a->sin6_port == b->sin6_port && memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr, 16) == 0))
                is_loop = 0;
        }
        else
        {
            return -1;
        }
    } while (is_loop);

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
    struct addrinfo *raddr = (struct addrinfo*) arg;

    return sendto(sock, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}
