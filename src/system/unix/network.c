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
#include <netdb.h>
#include <stdio.h>

#include "zenoh-pico/system/common.h"
#include "zenoh-pico/utils/private/logging.h"

/*------------------ IP helpers ------------------*/
int _zn_is_multicast_v4(struct sockaddr_in *addr)
{
    return IN_MULTICAST(addr->sin_addr.s_addr);
}

/*------------------ TCP sockets ------------------*/
void* _zn_create_tcp_endpoint(const char* s_addr, int port)
{
    struct sockaddr_in *addr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(s_addr);
    addr->sin_port = htons(port);

    return addr;
}

_zn_socket_result_t _zn_tcp_open(void *arg)
{
    struct sockaddr_in *raddr = (struct sockaddr_in*)arg;
    _zn_socket_result_t r;

    r.tag = _z_res_t_OK;
    r.value.socket = socket(AF_INET, SOCK_STREAM, 0);

    if (r.value.socket < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = r.value.socket;
        return r;
    }

    int flags = 1;
    if (setsockopt(r.value.socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) == -1)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = errno;
        close(r.value.socket);
        return r;
    }

    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = ZN_TRANSPORT_LEASE / 1000;
    if (setsockopt(r.value.socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) == -1)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = errno;
        close(r.value.socket);
        return r;
    }

#if defined(ZENOH_MACOS)
    setsockopt(r.value.socket, SOL_SOCKET, SO_NOSIGPIPE, (void *)0, sizeof(int));
#endif

    if (connect(r.value.socket, (struct sockaddr *)raddr, sizeof(*raddr)) < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_TX_CONNECTION;
        return r;
    }

    return r;
}

int _zn_tcp_close(_zn_socket_t sock)
{
    return shutdown(sock, SHUT_RDWR);
}

int _zn_tcp_read(_zn_socket_t sock, uint8_t *ptr, size_t len)
{
    return recv(sock, ptr, len, 0);
}

int _zn_tcp_read_exact(_zn_socket_t sock, uint8_t *ptr, size_t len)
{
    int n = len;
    int rb;

    do
    {
        rb = _zn_tcp_read(sock, ptr, n);
        if (rb < 0)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

int _zn_tcp_send(_zn_socket_t sock, const uint8_t *ptr, size_t len)
{
#if defined(ZENOH_LINUX)
    return send(sock, ptr, len, MSG_NOSIGNAL);
#else
    return send(sock, ptr, len, 0);
#endif
}

/*------------------ UDP sockets ------------------*/
void* _zn_create_udp_endpoint(const char* s_addr, int port)
{
    struct sockaddr_in *addr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(s_addr);
    addr->sin_port = htons(port);

    return addr;
}

_zn_socket_result_t _zn_udp_open(void* arg, clock_t tout)
{
    struct sockaddr_in *raddr = (struct sockaddr_in*)arg;
    _zn_socket_result_t r;

    r.tag = _z_res_t_OK;
    r.value.socket = socket(AF_INET, SOCK_DGRAM, 0);

    if (r.value.socket < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = r.value.socket;
        return r;
    }

    // FIXME: get value from configuration file
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = tout; // tout is passed in milliseconds
    if (setsockopt(r.value.socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(struct timeval)) == -1)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = errno;
        close(r.value.socket);
        return r;
    }

    if (setsockopt(r.value.socket, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(struct timeval)) == -1)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = errno;
        close(r.value.socket);
        return r;
    }

    struct sockaddr_in laddr;
    memset(&laddr, 0, sizeof(laddr));
    laddr.sin_family = AF_INET;
    laddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(r.value.socket, (struct sockaddr *)&laddr, sizeof(laddr)) < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        return r;
    }

    if (_zn_is_multicast_v4(raddr) == 1)
    {
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = raddr->sin_addr.s_addr;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(r.value.socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
        {
            r.tag = _z_res_t_ERR;
            r.value.error = _zn_err_t_MULTICAST_JOIN_FAILED;
            return r;
        }
    }

    return r;
}

int _zn_udp_close(_zn_socket_t sock)
{
    return close(sock);
}

int _zn_udp_read(_zn_socket_t sock, uint8_t *ptr, size_t len)
{
    return recv(sock, ptr, len, 0);
}

int _zn_udp_read_exact(_zn_socket_t sock, uint8_t *ptr, size_t len)
{
    int n = len;
    int rb;

    do
    {
        rb = _zn_udp_read(sock, ptr, n);
        if (rb < 0)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

int _zn_udp_send(_zn_socket_t sock, const uint8_t *ptr, size_t len, void *arg)
{
    struct sockaddr_in *raddr = (struct sockaddr_in*) arg;

    return sendto(sock, ptr, len, 0, (struct sockaddr *) raddr, sizeof(*raddr));
}
