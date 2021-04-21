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

#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <net/net_if.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "zenoh-pico/net/private/system.h"
#include "zenoh-pico/private/logging.h"
#include "zenoh-pico/private/iobuf.h"

typedef struct
{
    struct net_if *iface;
} iface_info_t;

void net_if_iterator_func(struct net_if *iface, void *user_data)
{
    iface_info_t *iface_info = user_data;

    if (iface_info->iface == NULL)
    {
        iface_info->iface = iface;
    }
}

/*------------------ Interfaces and sockets ------------------*/
char *_zn_select_scout_iface()
{
    // @TODO: improve network interface selection
    char host[NI_MAXHOST];

    iface_info_t iface_info;
    iface_info.iface = NULL;

    net_if_foreach(net_if_iterator_func, &iface_info);

    if (iface_info.iface != NULL)
    {
        struct net_addr addr = iface_info.iface->config.ip.ipv4->unicast->address;
        struct sockaddr_in sa = {.sin_family = addr.family, .sin_addr = addr.in_addr};

        getnameinfo((const struct sockaddr *)&sa,
                    sizeof(struct sockaddr_in),
                    host, NI_MAXHOST,
                    NULL, 0, NI_NUMERICHOST);
        _Z_DEBUG_VA("\tAddress: <%s>\n", host);

        char *result = strdup(host);
        return result;
    }

    return 0;
}

struct sockaddr_in *_zn_make_socket_address(const char *addr, int port)
{
    struct sockaddr_in *saddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(saddr, 0, sizeof(struct sockaddr_in));
    saddr->sin_family = AF_INET;
    saddr->sin_port = htons(port);

    if (inet_pton(AF_INET, addr, &(saddr->sin_addr)) <= 0)
    {
        free(saddr);
        return NULL;
    }

    return saddr;
}

_zn_socket_result_t _zn_create_udp_socket(const char *addr, int port, int timeout_usec)
{
    _zn_socket_result_t r;
    r.tag = _z_res_t_OK;

    _Z_DEBUG_VA("Binding UDP Socket to: %s:%d\n", addr, port);
    struct sockaddr_in saddr;

    r.value.socket = socket(PF_INET, SOCK_DGRAM, 0);

    if (r.value.socket < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = r.value.socket;
        r.value.socket = 0;
        return r;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);

    if (inet_pton(AF_INET, addr, &saddr.sin_addr) <= 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        r.value.socket = 0;
        return r;
    }

    if (bind(r.value.socket, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        r.value.socket = 0;
        return r;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_usec;
    if (setsockopt(r.value.socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(struct timeval)) == -1)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = errno;
        close(r.value.socket);
        r.value.socket = 0;
        return r;
    }

    // NOTE: SO_SNDTIMEO socket option is not supported in Zephyr
    return r;
}

_zn_socket_result_t _zn_open_tx_session(const char *locator)
{
    _zn_socket_result_t r;
    r.tag = _z_res_t_OK;
    char *l = strdup(locator);
    _Z_DEBUG_VA("Connecting to: %s:\n", locator);
    char *tx = strtok(l, "/");
    if (strcmp(tx, "tcp") != 0)
    {
        fprintf(stderr, "Locator provided is not for TCP\n");
        _exit(-1);
    }
    char *addr_name = strdup(strtok(NULL, ":"));
    char *s_port = strtok(NULL, ":");

    int status;
    char ip_addr[INET6_ADDRSTRLEN];
    struct sockaddr_in *remote;
    struct addrinfo *res;
    status = getaddrinfo(addr_name, s_port, NULL, &res);
    if (status == 0 && res != NULL)
    {
        void *addr;
        remote = (struct sockaddr_in *)res->ai_addr;
        addr = &(remote->sin_addr);
        inet_ntop(res->ai_family, addr, ip_addr, sizeof(ip_addr));
    }
    freeaddrinfo(res);

    int port;
    sscanf(s_port, "%d", &port);

    _Z_DEBUG_VA("Connecting to: %s:%d\n", addr_name, port);
    free(addr_name);
    free(l);
    struct sockaddr_in serv_addr;

    r.value.socket = socket(PF_INET, SOCK_STREAM, 0);

    if (r.value.socket < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = r.value.socket;
        r.value.socket = 0;
        return r;
    }

    // NOTE: SO_KEEPALIVE and SO_LINGER socket options are not supported in Zephyr

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr) <= 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        r.value.socket = 0;
        return r;
    }

    if (connect(r.value.socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_TX_CONNECTION;
        r.value.socket = 0;
        return r;
    }

    return r;
}

void _zn_close_tx_session(_zn_socket_t sock)
{
    shutdown(sock, 2);
}

/*------------------ Datagram ------------------*/
int _zn_send_dgram_to(_zn_socket_t sock, const _z_wbuf_t *wbf, const struct sockaddr *dest, socklen_t salen)
{
    _Z_DEBUG("Sending data on socket....\n");
    int wb = 0;
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++)
    {
        z_bytes_t a = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        int res = sendto(sock, a.val, a.len, 0, dest, salen);
        _Z_DEBUG_VA("Socket returned: %d\n", wb);
        if (res <= 0)
        {
            _Z_DEBUG_VA("Error while sending data over socket [%d]\n", wb);
            return -1;
        }
        wb += res;
    }
    return wb;
}

int _zn_recv_dgram_from(_zn_socket_t sock, _z_rbuf_t *rbf, struct sockaddr *from, socklen_t *salen)
{
    int rb = recvfrom(sock, _z_rbuf_get_wptr(rbf), _z_rbuf_space_left(rbf), 0, from, salen);
    if (rb > 0)
        _z_rbuf_set_wpos(rbf, _z_rbuf_get_wpos(rbf) + rb);

    return rb;
}

/*------------------ Receive ------------------*/
int _zn_recv_bytes(_zn_socket_t sock, uint8_t *ptr, size_t len)
{
    int n = len;
    int rb;

    do
    {
        rb = recv(sock, ptr, n, 0);
        if (rb == 0)
            return -1;
        n -= rb;
        ptr = ptr + (len - n);
    } while (n > 0);

    return 0;
}

int _zn_recv_rbuf(_zn_socket_t sock, _z_rbuf_t *rbf)
{
    int rb = recv(sock, _z_rbuf_get_wptr(rbf), _z_rbuf_space_left(rbf), 0);
    if (rb > 0)
        _z_rbuf_set_wpos(rbf, _z_rbuf_get_wpos(rbf) + rb);
    return rb;
}

/*------------------ Send ------------------*/
int _zn_send_wbuf(_zn_socket_t sock, const _z_wbuf_t *wbf)
{
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++)
    {
        z_bytes_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        int n = bs.len;
        int wb;
        do
        {
            _Z_DEBUG("Sending wbuf on socket...");
            wb = send(sock, bs.val, n, 0);
            _Z_DEBUG_VA(" sent %d bytes\n", wb);
            if (wb <= 0)
            {
                _Z_DEBUG_VA("Error while sending data over socket [%d]\n", wb);
                return -1;
            }
            n -= wb;
            bs.val += bs.len - n;
        } while (n > 0);
    }

    return 0;
}

// NOTE: iovec operations are not supported in Zephyr
