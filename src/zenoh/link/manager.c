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

#include "zenoh-pico/link/private/manager.h"
#include "zenoh-pico/system/common.h"

char* _zn_parse_port_segment(const char *address)
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

char* _zn_parse_address_segment(const char *address)
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

_zn_link_p_result_t _zn_open_link(const char *locator, clock_t tout)
{
    _zn_link_p_result_t r;
    r.tag = _z_res_t_OK;

    char *s_addr = NULL;
    char *s_port = NULL;

    _zn_endpoint_t *endpoint = _zn_endpoint_from_string(locator);
    if (endpoint == NULL)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        goto EXIT_OPEN_LINK;
    }

    s_port = _zn_parse_port_segment(endpoint->address);
    if (s_port == NULL)
    {
        s_port = (char*)malloc(sizeof(char) * strlen("7447"));
        strcpy(s_port, "7447");
    }

    s_addr = _zn_parse_address_segment(endpoint->address);
    if (s_addr == NULL)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        goto EXIT_OPEN_LINK;
    }

    // Create transport link
    _zn_link_t *link = NULL;
    if (strcmp(endpoint->protocol, TCP_SCHEMA) == 0)
    {
        link = _zn_new_link_unicast_tcp(s_addr, s_port);
    }
    else if (strcmp(endpoint->protocol, UDP_SCHEMA) == 0)
    {
        link = _zn_new_link_unicast_udp(s_addr, s_port);
    }
    else
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        goto EXIT_OPEN_LINK;
    }

    // Open transport link for communication
    _zn_socket_result_t r_sock = link->open_f(link, tout);
    if (r_sock.tag == _z_res_t_ERR)
    {
        _zn_close_link(link);
        free(link);

        r.tag = _z_res_t_ERR;
        r.value.error = r_sock.value.error;
        goto EXIT_OPEN_LINK;
    }

    link->sock = r_sock.value.socket;
    r.value.link = link;

EXIT_OPEN_LINK:
    free(s_port);
    free(s_addr);

    return r;
}

void _zn_close_link(_zn_link_t *link)
{
    link->close_f(link);
}
