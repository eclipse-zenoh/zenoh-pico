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

#include <string.h>
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/config/tcp.h"
#include "zenoh-pico/system/link/tcp.h"

#if ZN_LINK_TCP == 1

z_str_t _zn_parse_port_segment_tcp(z_str_t address)
{
    z_str_t p_start = strrchr(address, ':');
    if (p_start == NULL)
        return NULL;
    p_start++;

    z_str_t p_end = &address[strlen(address)];

    int len = p_end - p_start;
    z_str_t port = (z_str_t)malloc((len + 1) * sizeof(char));
    strncpy(port, p_start, len);
    port[len] = '\0';

    return port;
}

z_str_t _zn_parse_address_segment_tcp(z_str_t address)
{
    z_str_t p_start = &address[0];
    z_str_t p_end = strrchr(address, ':');

    // IPv6
    if (*p_start == '[' && *(p_end - 1) == ']')
    {
        p_start++;
        p_end--;
        int len = p_end - p_start;
        z_str_t ip6_addr = (z_str_t)malloc((len + 1) * sizeof(char));
        strncpy(ip6_addr, p_start, len);
        ip6_addr[len] = '\0';

        return ip6_addr;
    }
    // IPv4
    else
    {
        int len = p_end - p_start;
        z_str_t ip4_addr_or_domain = (z_str_t)malloc((len + 1) * sizeof(char));
        strncpy(ip4_addr_or_domain, p_start, len);
        ip4_addr_or_domain[len] = '\0';

        return ip4_addr_or_domain;
    }

    return NULL;
}

int _zn_f_link_open_tcp(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    clock_t timeout = ZN_CONFIG_SOCKET_TIMEOUT_DEFAULT;
    z_str_t tout = _z_str_intmap_get(&self->endpoint.config, TCP_CONFIG_TOUT_KEY);
    if (tout != NULL)
        timeout = strtof(tout, NULL);

    self->socket.tcp.sock = _zn_open_tcp(self->socket.tcp.raddr, timeout);
    if (self->socket.tcp.sock < 0)
        goto ERR;

    return 0;

ERR:
    return -1;
}

int _zn_f_link_listen_tcp(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    self->socket.tcp.sock = _zn_listen_tcp(self->socket.tcp.raddr);
    if (self->socket.tcp.sock < 0)
        goto ERR;

    return 0;

ERR:
    return -1;
}

void _zn_f_link_close_tcp(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    _zn_close_tcp(self->socket.tcp.sock);
}

void _zn_f_link_free_tcp(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    _zn_free_endpoint_tcp(self->socket.tcp.raddr);
}

size_t _zn_f_link_write_tcp(const void *arg, const uint8_t *ptr, size_t len)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_send_tcp(self->socket.tcp.sock, ptr, len);
}

size_t _zn_f_link_write_all_tcp(const void *arg, const uint8_t *ptr, size_t len)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_send_tcp(self->socket.tcp.sock, ptr, len);
}

size_t _zn_f_link_read_tcp(const void *arg, uint8_t *ptr, size_t len, z_bytes_t *addr)
{
    (void) (addr);
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_read_tcp(self->socket.tcp.sock, ptr, len);
}

size_t _zn_f_link_read_exact_tcp(const void *arg, uint8_t *ptr, size_t len, z_bytes_t *addr)
{
    (void) (addr);
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_read_exact_tcp(self->socket.tcp.sock, ptr, len);
}

uint16_t _zn_get_link_mtu_tcp(void)
{
    // Maximum MTU for TCP
    return 65535;
}

_zn_link_t *_zn_new_link_tcp(_zn_endpoint_t endpoint)
{
    _zn_link_t *lt = (_zn_link_t *)malloc(sizeof(_zn_link_t));

    lt->is_reliable = 1;
    lt->is_streamed = 1;
    lt->is_multicast = 0;
    lt->mtu = _zn_get_link_mtu_tcp();

    lt->endpoint = endpoint;

    lt->socket.tcp.sock = -1;
    z_str_t s_addr = _zn_parse_address_segment_tcp(endpoint.locator.address);
    z_str_t s_port = _zn_parse_port_segment_tcp(endpoint.locator.address);
    lt->socket.tcp.raddr = _zn_create_endpoint_tcp(s_addr, s_port);
    free(s_addr);
    free(s_port);

    lt->open_f = _zn_f_link_open_tcp;
    lt->listen_f = _zn_f_link_listen_tcp;
    lt->close_f = _zn_f_link_close_tcp;
    lt->free_f = _zn_f_link_free_tcp;

    lt->write_f = _zn_f_link_write_tcp;
    lt->write_all_f = _zn_f_link_write_all_tcp;
    lt->read_f = _zn_f_link_read_tcp;
    lt->read_exact_f = _zn_f_link_read_exact_tcp;

    return lt;
}
#endif
