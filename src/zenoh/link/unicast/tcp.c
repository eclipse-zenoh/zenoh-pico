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

#include "zenoh-pico/system/common.h"
#include "zenoh-pico/link/private/manager.h"

char* _zn_parse_port_segment_tcp(const char *address)
{
    const char *p_init = strrchr(address, ':');
    if (p_init == NULL)
        return NULL;
    ++p_init;

    const char *p_end = &address[strlen(address)];

    int len = p_end - p_init;
    char *port = (char*)malloc((len + 1) * sizeof(char));
    strncpy(port, p_init, len);
    port[len] = '\0';

    return port;
}

char* _zn_parse_address_segment_tcp(const char *address)
{
    const char *p_init = &address[0];
    const char *p_end = strrchr(address, ':');

    if (*p_init == '[' && *(--p_end) == ']')
    {
       int len = p_end - ++p_init;
       char *ip6_addr = (char*)malloc((len + 1) * sizeof(char));
       strncpy(ip6_addr, p_init, len);
       ip6_addr[len] = '\0';

       return ip6_addr;
    }
    else
    {
       int len = p_end - p_init;
       char *ip4_addr_or_domain = (char*)malloc((len + 1) * sizeof(char));
       strncpy(ip4_addr_or_domain, p_init, len);
       ip4_addr_or_domain[len] = '\0';

       return ip4_addr_or_domain;
    }

    return NULL;
}

_zn_socket_result_t _zn_f_link_open_unicast_tcp(void *arg, const clock_t tout)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_open_unicast_tcp(self->endpoint_syscall);
}

int _zn_f_link_close_unicast_tcp(void *arg)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_close_tcp(self->sock);
}

void _zn_f_link_release_unicast_tcp(void *arg)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    _zn_release_endpoint_tcp(self->endpoint_syscall);
}

size_t _zn_f_link_write_unicast_tcp(void *arg, const uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_send_tcp(self->sock, ptr, len);
}

size_t _zn_f_link_write_all_unicast_tcp(void *arg, const uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_send_tcp(self->sock, ptr, len);
}

size_t _zn_f_link_read_unicast_tcp(void *arg, uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_read_tcp(self->sock, ptr, len);
}

size_t _zn_f_link_read_exact_unicast_tcp(void *arg, uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_read_exact_tcp(self->sock, ptr, len);
}

size_t _zn_get_link_mtu_unicast_tcp()
{
    // TODO
    return -1;
}

_zn_link_t *_zn_new_link_unicast_tcp(_zn_endpoint_t *endpoint)
{
    _zn_link_t *lt = (_zn_link_t *)malloc(sizeof(_zn_link_t));
    lt->is_reliable = 1;
    lt->is_streamed = 1;
    lt->mtu = _zn_get_link_mtu_unicast_tcp();

    char *s_addr = _zn_parse_address_segment_tcp(endpoint->address);
    char *s_port = _zn_parse_port_segment_tcp(endpoint->address);
    lt->endpoint_syscall = _zn_create_endpoint_tcp(s_addr, s_port);
    lt->endpoint = endpoint;

    lt->open_f = _zn_f_link_open_unicast_tcp;
    lt->close_f = _zn_f_link_close_unicast_tcp;
    lt->release_f = _zn_f_link_release_unicast_tcp;

    lt->write_f = _zn_f_link_write_unicast_tcp;
    lt->write_all_f = _zn_f_link_write_all_unicast_tcp;
    lt->read_f = _zn_f_link_read_unicast_tcp;
    lt->read_exact_f = _zn_f_link_read_exact_unicast_tcp;

    free(s_addr);
    free(s_port);

    return lt;
}
