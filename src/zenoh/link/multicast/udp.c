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
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/config/udp.h"
#include "zenoh-pico/utils/properties.h"

z_str_t _zn_parse_port_segment_udp_multicast(const z_str_t address)
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

z_str_t _zn_parse_address_segment_udp_multicast(const z_str_t address)
{
    z_str_t p_start = &address[0];
    z_str_t p_end = strrchr(address, ':');

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

_zn_socket_result_t _zn_f_link_open_udp_multicast(void *arg, const clock_t tout)
{
    _zn_link_t *self = (_zn_link_t *)arg;
    _zn_socket_result_t r;
    r.tag = _z_res_t_OK;

    const z_str_t iface = _zn_state_get(&self->endpoint.config, UDP_CONFIG_MULTICAST_IFACE_KEY);
    if (iface == NULL)
        goto _ZN_F_LINK_OPEN_UDP_MULTICAST_ERROR;

    self->sock = _zn_open_udp_multicast(self->raddr, &self->laddr, tout, iface);
    if (self->sock < 0)
        goto _ZN_F_LINK_OPEN_UDP_MULTICAST_ERROR;
    r.value.socket = self->sock;
    return r;

_ZN_F_LINK_OPEN_UDP_MULTICAST_ERROR:
    r.tag = _z_res_t_ERR;
    r.value.error = _zn_err_t_OPEN_TRANSPORT_FAILED;
    return r;
}

_zn_socket_result_t _zn_f_link_listen_udp_multicast(void *arg, const clock_t tout)
{
    _zn_link_t *self = (_zn_link_t *)arg;
    _zn_socket_result_t r;
    r.tag = _z_res_t_OK;

    const z_str_t iface = _zn_state_get(&self->endpoint.config, UDP_CONFIG_MULTICAST_IFACE_KEY);
    if (iface == NULL)
        goto _ZN_F_LINK_LISTEN_UDP_MULTICAST_ERROR;

    self->sock = _zn_listen_udp_multicast(self->raddr, tout, iface);
    if (self->sock < 0)
        goto _ZN_F_LINK_LISTEN_UDP_MULTICAST_ERROR;

    self->mcast_send_sock = _zn_open_udp_multicast(self->raddr, &self->laddr, tout, iface);
    if (self->mcast_send_sock < 0)
        goto _ZN_F_LINK_LISTEN_UDP_MULTICAST_ERROR;

    r.value.socket = self->sock; // FIXME: we do not need to return it anymore
    return r;

_ZN_F_LINK_LISTEN_UDP_MULTICAST_ERROR:
    r.tag = _z_res_t_ERR;
    r.value.error = _zn_err_t_OPEN_TRANSPORT_FAILED;
    return r;
}

void _zn_f_link_close_udp_multicast(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    _zn_close_udp_multicast(self->sock, self->mcast_send_sock, self->raddr);
}

void _zn_f_link_free_udp_multicast(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    _zn_free_endpoint_udp(self->raddr);
    _zn_free_endpoint_udp(self->laddr);
}

size_t _zn_f_link_write_udp_multicast(const void *arg, const uint8_t *ptr, size_t len)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_send_udp_multicast(self->mcast_send_sock, ptr, len, self->raddr);
}

size_t _zn_f_link_write_all_udp_multicast(const void *arg, const uint8_t *ptr, size_t len)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_send_udp_multicast(self->mcast_send_sock, ptr, len, self->raddr);
}

size_t _zn_f_link_read_udp_multicast(const void *arg, uint8_t *ptr, size_t len)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_read_udp_multicast(self->sock, ptr, len, self->laddr);
}

size_t _zn_f_link_read_exact_udp_multicast(const void *arg, uint8_t *ptr, size_t len)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_read_exact_udp_multicast(self->sock, ptr, len, self->laddr);
}

size_t _zn_get_link_mtu_udp_multicast()
{
    // TODO
    return -1;
}

_zn_link_t *_zn_new_link_udp_multicast(_zn_endpoint_t endpoint)
{
    _zn_link_t *lt = (_zn_link_t *)malloc(sizeof(_zn_link_t));

    lt->is_reliable = 0;
    lt->is_streamed = 0;
    lt->is_multicast = 1;
    lt->mtu = _zn_get_link_mtu_udp_multicast();

    z_str_t s_addr = _zn_parse_address_segment_udp_multicast(endpoint.locator.address);
    z_str_t s_port = _zn_parse_port_segment_udp_multicast(endpoint.locator.address);
    lt->raddr = _zn_create_endpoint_udp(s_addr, s_port);
    lt->laddr = NULL;
    lt->endpoint = endpoint;

    lt->open_f = _zn_f_link_open_udp_multicast;
    lt->listen_f = _zn_f_link_listen_udp_multicast;
    lt->close_f = _zn_f_link_close_udp_multicast;
    lt->free_f = _zn_f_link_free_udp_multicast;

    lt->write_f = _zn_f_link_write_udp_multicast;
    lt->write_all_f = _zn_f_link_write_all_udp_multicast;
    lt->read_f = _zn_f_link_read_udp_multicast;
    lt->read_exact_f = _zn_f_link_read_exact_udp_multicast;

    free(s_addr);
    free(s_port);

    return lt;
}
