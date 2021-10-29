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

#include <errno.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>

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

    int flags = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)
    {
        close(sock);
        return -1;
    }

    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = ZN_TRANSPORT_LEASE / 1000;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)
    {
        close(sock);
        return -1;
    }
#if defined(ZENOH_MACOS)
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void *)0, sizeof(int));
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

int _zn_read_tcp(int sock, uint8_t *ptr, size_t len)
{
    return recv(sock, ptr, len, 0);
}

int _zn_read_exact_tcp(int sock, uint8_t *ptr, size_t len)
{
    int n = len;
    int rb;

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

int _zn_send_tcp(int sock, const uint8_t *ptr, size_t len)
{
#if defined(ZENOH_LINUX)
    return send(sock, ptr, len, MSG_NOSIGNAL);
#else
    return send(sock, ptr, len, 0);
#endif
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

int _zn_read_udp_unicast(int sock, uint8_t *ptr, size_t len)
{
    struct sockaddr *addr;
    unsigned int addrlen;

    size_t rb = recvfrom(sock, ptr, len, 0,
                    (struct sockaddr *)addr, &addrlen);

    return rb;
}

int _zn_read_exact_udp_unicast(int sock, uint8_t *ptr, size_t len)
{
    int n = len;
    int rb;

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

int _zn_send_udp_unicast(int sock, const uint8_t *ptr, size_t len, void *arg)
{
    struct addrinfo *raddr = (struct addrinfo*) arg;

    return sendto(sock, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}

int _zn_open_udp_multicast(void *arg_1, void **arg_2, const clock_t tout, const char *iface)
{
    struct addrinfo *raddr = (struct addrinfo *)arg_1;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0)
        return -1;

    // Set the interface to bind to
    unsigned int ifindex = if_nametoindex(iface);
    if (raddr->ai_family == AF_INET)
    {
        if (setsockopt(sock, IPPROTO_IP, IP_BOUND_IF, &ifindex, sizeof(ifindex)) < 0)
            return -1;
    }
    else if (raddr->ai_family == AF_INET6)
    {
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_BOUND_IF, &ifindex, sizeof(ifindex)) < 0)
            return -1;
    }
    else
        return -1;

    // Explicitly bind is needed, because we need to keep track of the addrinfo
    // to drop looped multicast messages
    struct sockaddr *laddr = (struct sockaddr *)malloc(sizeof(struct sockaddr));
    memset(laddr, 0, sizeof(struct sockaddr));
    if (raddr->ai_family == AF_INET)
    {
        struct sockaddr_in *c_laddr = (struct sockaddr_in *)laddr;
        c_laddr->sin_family = AF_INET;
        c_laddr->sin_addr.s_addr = INADDR_ANY;
        c_laddr->sin_port = htons(INADDR_ANY);
    }
    else if (raddr->ai_family == AF_INET6)
    {
        struct sockaddr_in6 *c_laddr = (struct sockaddr_in6 *)laddr;
        c_laddr->sin6_family = AF_INET6;
        c_laddr->sin6_addr = in6addr_any;
        c_laddr->sin6_port = htons(INADDR_ANY);
        c_laddr->sin6_scope_id = if_nametoindex(iface);
    }
    else
        return -1;

    if (bind(sock, laddr, sizeof(struct sockaddr)) < 0)
        return -1;

    unsigned int sockaddr_len = sizeof(struct sockaddr);
    if (getsockname(sock, laddr, &sockaddr_len) < -1)
        return -1;

    *arg_2 = laddr;
    return sock;
}

int _zn_listen_udp_multicast(void *arg, const clock_t tout, const char *iface)
{
    struct addrinfo *raddr = (struct addrinfo*)arg;

    int sock = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (sock < 0)
        goto EXIT_MULTICAST_LISTEN_ERROR;

    int optflag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optflag, sizeof(optflag)) < 0)
        goto EXIT_MULTICAST_LISTEN_ERROR;

    // Set the interface to bind to
    unsigned int ifindex = if_nametoindex(iface);
    if (raddr->ai_family == AF_INET)
    {
        if (setsockopt(sock, IPPROTO_IP, IP_BOUND_IF, &ifindex, sizeof(ifindex)) < 0)
            goto EXIT_MULTICAST_LISTEN_ERROR;
    }
    else if (raddr->ai_family == AF_INET6)
    {
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_BOUND_IF, &ifindex, sizeof(ifindex)) < 0)
            goto EXIT_MULTICAST_LISTEN_ERROR;
    }
    else
        goto EXIT_MULTICAST_LISTEN_ERROR;

    char s_port[6]; // String representation of a port has maximum 5 characters
    if (raddr->ai_family == AF_INET)
        sprintf(s_port, "%d", ntohs(((struct sockaddr_in*)raddr->ai_addr)->sin_port));
    else if (raddr->ai_family == AF_INET6)
        sprintf(s_port, "%d", ntohs(((struct sockaddr_in6*)raddr->ai_addr)->sin6_port));
    else
        goto EXIT_MULTICAST_LISTEN_ERROR;

    // Bind socket
    struct addrinfo hints;
    struct addrinfo *laddr = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = raddr->ai_family;
    hints.ai_socktype = raddr->ai_socktype;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = raddr->ai_protocol;
    if (getaddrinfo(NULL, s_port, &hints, &laddr) == 0)
    {
        if (bind(sock, laddr->ai_addr, laddr->ai_addrlen) < 0)
        {
            freeaddrinfo(laddr);
            goto EXIT_MULTICAST_LISTEN_ERROR;
        }
    }
    else
        goto EXIT_MULTICAST_LISTEN_ERROR;

    // Join the multicast group
    if (raddr->ai_family == AF_INET)
    {
//        // This option has no effect, since we use two sockets:
//        // one to send and another to receive the multicast packets
//        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &optflag, sizeof(optflag)) < 0)
//            goto EXIT_MULTICAST_LISTEN_ERROR;

        if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &((struct sockaddr_in*)laddr->ai_addr)->sin_addr, sizeof(struct in_addr)) < 0)
        {
            freeaddrinfo(laddr);
            goto EXIT_MULTICAST_LISTEN_ERROR;
        }
        freeaddrinfo(laddr);

        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = ((struct sockaddr_in*)raddr->ai_addr)->sin_addr.s_addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                       &mreq, sizeof(mreq)) < 0)
            goto EXIT_MULTICAST_LISTEN_ERROR;
    }
    else if(raddr->ai_family == AF_INET6)
    {
        freeaddrinfo(laddr);

//        // This option has no effect, since we use two sockets:
//        // one to send and another to receive the multicast packets
//        if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &optflag, sizeof(optflag)) < 0)
//            goto EXIT_MULTICAST_LISTEN_ERROR;

        if(setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0)
            goto EXIT_MULTICAST_LISTEN_ERROR;

        struct ipv6_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        memcpy(&mreq.ipv6mr_multiaddr,
                   &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr,
                   sizeof(struct in6_addr));
        mreq.ipv6mr_interface = ifindex;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0)
            goto EXIT_MULTICAST_LISTEN_ERROR;
    }
    else
    {
        freeaddrinfo(laddr);
        goto EXIT_MULTICAST_LISTEN_ERROR;
    }

    return sock;

EXIT_MULTICAST_LISTEN_ERROR:
    close(sock);

    return -1;
}

int _zn_close_udp_multicast(int sock, void *arg)
{
    struct addrinfo *raddr = (struct addrinfo*)arg;

    if (raddr->ai_family == AF_INET)
    {
        struct ip_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = ((struct sockaddr_in*)raddr->ai_addr)->sin_addr.s_addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    }
    else if (raddr->ai_family == AF_INET6)
    {
        struct ipv6_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        memcpy(&mreq.ipv6mr_multiaddr,
                   &((struct sockaddr_in6 *)raddr->ai_addr)->sin6_addr,
                   sizeof(struct in6_addr));
        //mreq.ipv6mr_interface = ifindex;
        setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &mreq, sizeof(mreq));
    }

    return close(sock);
}

int _zn_read_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg)
{
    struct sockaddr *laddr = (struct sockaddr*)arg;
    struct sockaddr raddr;
    unsigned int addrlen = sizeof(struct sockaddr);;

    size_t rb = recvfrom(sock, ptr, len, 0,
                    (struct sockaddr *)&raddr, &addrlen);

    if (laddr->sa_family == AF_INET)
    {
        struct sockaddr_in *a = ((struct sockaddr_in *)laddr);
        struct sockaddr_in *b = ((struct sockaddr_in *)&raddr);
// FIXME:        if (a->sin_port == b->sin_port && memcmp(&a->sin_addr, &b->sin_addr, sizeof(struct in_addr)) == 0)
        if (a->sin_port == b->sin_port)
            return 0;
    }
    else if (laddr->sa_family == AF_INET6)
    {
        struct sockaddr_in6 *a = ((struct sockaddr_in6 *)laddr);
        struct sockaddr_in6 *b = ((struct sockaddr_in6 *)&raddr);
// FIXME:        if (a->sin6_port == b->sin6_port && memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(struct in6_addr)) == 0)
        if (a->sin6_port == b->sin6_port)
            return 0;
    }
    else
    {
        return -1;
    }

    return rb;
}

int _zn_read_exact_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg)
{
    int n = len;
    int rb;

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

int _zn_send_udp_multicast(int sock, const uint8_t *ptr, size_t len, void *arg)
{
    struct addrinfo *raddr = (struct addrinfo*) arg;

    return sendto(sock, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}
