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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zenoh-pico/transport/private/manager.h"
#include "zenoh-pico/system/common.h"

_zn_link_p_result_t _zn_open_link(const char* locator, clock_t tout)
{
    _zn_link_p_result_t r;
    r.tag = _z_res_t_OK;

    // FIXME: IPv6 fail to be parsed due to :
    //        Other locators might not have a port
    // Parse locator
    char *l = strdup(locator);
    char *protocol = strtok(l, "/");
    char *s_addr = strdup(strtok(NULL, ":"));
    char *s_port = strtok(NULL, ":");

    _zn_link_t *link = NULL;
    // TODO optimization: hash the scheme
    if (strcmp(protocol, TCP_SCHEMA) == 0)
    {
        link = _zn_new_tcp_link(s_addr, s_port);
    }
    else if (strcmp(protocol, UDP_SCHEMA) == 0)
    {
        link = _zn_new_udp_link(s_addr, s_port);
    }

    _zn_socket_result_t r_sock = link->open_f(link, tout);
    if (r_sock.tag == _z_res_t_ERR)
    {
        _zn_close_link(link);
        r.tag = _z_res_t_ERR;
        r.value.error = r_sock.value.error;
    }

    link->sock = r_sock.value.socket;
    r.value.link = link;

    free(l);
    free(s_addr);
    return r;
}

void _zn_close_link(_zn_link_t *link)
{
    link->close_f(link);
}
