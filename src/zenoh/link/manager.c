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

char* _zn_parse_protocol_segment(const char* locator)
{
    char *pos = strpbrk(locator, "/");
    if (pos == NULL)
        return NULL;

    int len = pos - locator;
    char *protocol = (char*)malloc((len + 1) * sizeof(char));
    strncpy(protocol, locator, len);
    protocol[len] = '\0';

    return protocol;
}

char* _zn_parse_port_segment(const char* locator)
{
    char *pos = strrchr(locator, ':');
    if (pos == NULL)
        return NULL;

    int len = strlen(locator) - (pos - locator + 1);
    char *port = (char*)malloc((len + 1) * sizeof(char));
    strncpy(port, ++pos, len);
    port[len] = '\0';

    return port;
}

char* _zn_parse_address_segment(const char* locator)
{
    char* p = _zn_parse_protocol_segment(locator);
    char* a = _zn_parse_port_segment(locator);
    if (p == NULL || a == NULL)
        return NULL;

    if (*(locator + strlen(p) + 1) == '[' && *(locator + strlen(locator) - strlen(a) - 2) == ']')
    {
       int len = strlen(locator) - strlen(a) - strlen(p) - 4;
       char* ip6_addr = (char*)malloc((len + 1) * sizeof(char));
       strncpy(ip6_addr, locator + strlen(p) + 2, len);
       ip6_addr[len] = '\0';

       return ip6_addr;
    }
    else
    {
       int len = strlen(locator) - strlen(a) - strlen(p) - 2;
       char* ip4_addr_or_domain = (char*)malloc((len + 1) * sizeof(char));
       strncpy(ip4_addr_or_domain, locator + strlen(p) + 1, len);
       ip4_addr_or_domain[len] = '\0';

       return ip4_addr_or_domain;
    }

    return NULL;
}

_zn_link_p_result_t _zn_open_link(const char* locator, clock_t tout)
{
    _zn_link_p_result_t r;
    r.tag = _z_res_t_OK;

    // Parse locator
    char *protocol = _zn_parse_protocol_segment(locator);
    char *s_port = _zn_parse_port_segment(locator);
    char *s_addr = _zn_parse_address_segment(locator);
    if (protocol == NULL || s_port == NULL || s_addr == NULL)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        goto EXIT_OPEN_LINK;
    }

    // Create transport link
    _zn_link_t *link = NULL;
    if (strcmp(protocol, TCP_SCHEMA) == 0)
    {
        link = _zn_new_tcp_link(s_addr, s_port);
    }
    else if (strcmp(protocol, UDP_SCHEMA) == 0)
    {
        link = _zn_new_udp_link(s_addr, s_port);
    }

    // Open transport link for communication
    _zn_socket_result_t r_sock = link->open_f(link, tout);
    if (r_sock.tag == _z_res_t_ERR)
    {
        _zn_close_link(link);
        r.tag = _z_res_t_ERR;
        r.value.error = r_sock.value.error;
    }

    link->sock = r_sock.value.socket;
    r.value.link = link;

EXIT_OPEN_LINK:
    free(protocol);
    free(s_port);
    free(s_addr);

    return r;
}

void _zn_close_link(_zn_link_t *link)
{
    link->close_f(link);
}
