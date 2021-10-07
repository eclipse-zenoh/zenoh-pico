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
    struct addrinfo *addr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;       // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_addr, port, &hints, &addr) < 0)
        return NULL;

    freeaddrinfo(addr->ai_next);

    return addr;
}

void* _zn_create_endpoint_udp(const char *s_addr, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *addr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;       // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(s_addr, port, &hints, &addr) < 0)
        return NULL;

    freeaddrinfo(addr->ai_next);
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
_zn_socket_result_t _zn_open_tcp(void *arg)
{
    struct addrinfo *raddr = (struct addrinfo*)arg;
    _zn_socket_result_t r;
    r.tag = _z_res_t_OK;

    r.value.socket = socket(raddr->ai_family, raddr->ai_socktype, raddr->ai_protocol);
    if (r.value.socket < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = r.value.socket;
        return r;
    }

#if LWIP_SO_LINGER == 1
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = ZN_TRANSPORT_LEASE / 1000;
    if (setsockopt(r.value.socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = errno;
        close(r.value.socket);
        return r;
    }
#endif

    struct addrinfo *it;
    for (it = raddr; it != NULL; it = it->ai_next)
    {
        if (connect(r.value.socket, it->ai_addr, it->ai_addrlen) < 0)
        {
            if (it->ai_next == NULL)
            {
                r.tag = _z_res_t_ERR;
                r.value.error = _zn_err_t_TX_CONNECTION;
                return r;
            }
        }
        else
        {
            break;
        }
    }

    return r;
}

int _zn_close_tcp(_zn_socket_t sock)
{
    return shutdown(sock, SHUT_RDWR);
}

int _zn_read_tcp(_zn_socket_t sock, uint8_t *ptr, size_t len)
{
    return recv(sock, ptr, len, 0);
}

int _zn_read_exact_tcp(_zn_socket_t sock, uint8_t *ptr, size_t len)
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

int _zn_send_tcp(_zn_socket_t sock, const uint8_t *ptr, size_t len)
{
    return send(sock, ptr, len, 0);
}

/*------------------ UDP sockets ------------------*/
_zn_socket_result_t _zn_open_udp(void *arg, const clock_t tout)
{
    struct addrinfo *raddr = (struct addrinfo*)arg;
    _zn_socket_result_t r;
    r.tag = _z_res_t_OK;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = raddr->ai_family;
    hints.ai_socktype = raddr->ai_socktype;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = raddr->ai_protocol;

    struct addrinfo *laddr;
    if (getaddrinfo(NULL, "0", &hints, &laddr) != 0)  // port 0 --> random
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        return r;
    }

    r.value.socket = socket(laddr->ai_family, laddr->ai_socktype, laddr->ai_protocol);
    if (r.value.socket < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = r.value.socket;
        return r;
    }

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

    freeaddrinfo(laddr);
    return r;
}

int _zn_close_udp(_zn_socket_t sock)
{
    return close(sock);
}

int _zn_read_udp(_zn_socket_t sock, uint8_t *ptr, size_t len)
{
    return recv(sock, ptr, len, 0);
}

int _zn_read_exact_udp(_zn_socket_t sock, uint8_t *ptr, size_t len)
{
    int n = len;
    int rb;

    do
    {
        rb = _zn_read_udp(sock, ptr, n);
        if (rb < 0)
            return rb;

        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return len;
}

int _zn_send_udp(_zn_socket_t sock, const uint8_t *ptr, size_t len, void *arg)
{
    struct addrinfo *raddr = (struct addrinfo*) arg;

    return sendto(sock, ptr, len, 0, raddr->ai_addr, raddr->ai_addrlen);
}
