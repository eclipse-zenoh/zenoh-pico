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

#include <arpa/inet.h>
#include <errno.h>
//#include <ifaddrs.h>
#include <netdb.h>
//#include <net/if.h>
#include <unistd.h>
#include "zenoh-pico/system/common.h"
#include "zenoh-pico/utils/private/logging.h"

/*------------------ Interfaces and sockets ------------------*/
char *_zn_select_scout_iface()
{
//    // @TODO: improve network interface selection
//    char *eth_prefix = "en";
//    char *lo_prefix = "lo";
//    size_t len = 2;
//    char *loopback = 0;
//    char *iface = 0;
//    struct ifaddrs *ifap;
//    struct ifaddrs *current;
//    char host[NI_MAXHOST];
//
//    if (getifaddrs(&ifap) == -1)
//    {
//        return 0;
//    }
//    else
//    {
//        current = ifap;
//        do
//        {
//            if (current->ifa_addr != 0 && current->ifa_addr->sa_family == AF_INET)
//            {
//                if (memcmp(current->ifa_name, eth_prefix, len) == 0)
//                {
//                    if ((current->ifa_flags & (IFF_MULTICAST | IFF_UP | IFF_RUNNING)) && !(current->ifa_flags & IFF_PROMISC))
//                    {
//                        getnameinfo(current->ifa_addr,
//                                    sizeof(struct sockaddr_in),
//                                    host, NI_MAXHOST,
//                                    NULL, 0, NI_NUMERICHOST);
//                        _Z_DEBUG_VA("\t-- Interface: %s\tAddress: <%s>\n", current->ifa_name, host);
//                        iface = host;
//                    }
//                }
//                else if (memcmp(current->ifa_name, lo_prefix, len) == 0)
//                {
//                    if ((current->ifa_flags & (IFF_UP | IFF_RUNNING)) && !(current->ifa_flags & IFF_PROMISC))
//                    {
//                        getnameinfo(current->ifa_addr,
//                                    sizeof(struct sockaddr_in),
//                                    host, NI_MAXHOST,
//                                    NULL, 0, NI_NUMERICHOST);
//                        _Z_DEBUG_VA("\t-- Interface: %s\tAddress: <%s>\n", current->ifa_name, host);
//                        loopback = host;
//                    }
//                }
//            }
//            current = current->ifa_next;
//        } while ((iface == 0) && (current != 0));
//    }
//    char *result = strdup((iface != 0) ? iface : loopback);
//    freeifaddrs(ifap);
//    return result;
    return NULL;
}


/*------------------ TCP sockets ------------------*/
_zn_socket_result_t _zn_tcp_open(const char* s_addr, int port)
{
    _zn_socket_result_t r;
    char ip_addr[INET6_ADDRSTRLEN];
    struct sockaddr_in *remote;
    struct addrinfo *res;
    int status;

    status = getaddrinfo(s_addr, NULL, NULL, &res);
    if (status == 0 && res != NULL)
    {
        void *addr;
        remote = (struct sockaddr_in *)res->ai_addr;
        addr = &(remote->sin_addr);
        inet_ntop(res->ai_family, addr, ip_addr, sizeof(ip_addr));
    }
    freeaddrinfo(res);

    _Z_DEBUG_VA("Connecting to: %s:%d\n", s_name, port);
    struct sockaddr_in serv_addr;

    r.tag = _z_res_t_OK;
    r.value.socket = socket(PF_INET, SOCK_STREAM, 0);

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

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr) <= 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        return r;
    }

    if (connect(r.value.socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
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
        if (rb == 0)
            return -1;
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
_zn_socket_result_t _zn_udp_open(const char* s_addr, int port)
{
    _zn_socket_result_t r;

    r.tag = _z_res_t_OK;
    r.value.socket = socket(PF_INET, SOCK_DGRAM, 0);

    if (r.value.socket < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = r.value.socket;
        return r;
    }

    struct sockaddr_in laddr;
    memset(&laddr, 0, sizeof(s_addr));
    laddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, s_addr, &laddr.sin_addr) <= 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        return r;
    }

    // FIXME: get value from configuration file
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
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

    if (bind(r.value.socket, (struct sockaddr *)&laddr, sizeof(laddr)) < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        return r;
    }

    // Using connect for UDP is just binding the socket the dst address
    // Only dgrams to and from ip/port will be received using recv and send
    laddr.sin_port = htons(port);
    if (connect(r.value.socket, (struct sockaddr *)&laddr, sizeof(laddr)) < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_TX_CONNECTION;
        return r;
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

int _zn_udp_send(_zn_socket_t sock, const uint8_t *ptr, size_t len)
{
    return send(sock, ptr, len, 0);
}
