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

_zn_link_t *_zn_open_link(const char* locator, clock_t tout)
{
    // Parse locator
    char *l = strdup(locator);
    char *protocol = strtok(l, "/");
    char *s_addr = strdup(strtok(NULL, ":"));
    char *s_port = strtok(NULL, ":");
    int port;
    sscanf(s_port, "%d", &port);

    _zn_link_t *link = NULL;
    // TODO optimization: hash the scheme
    if (strcmp(protocol, TCP_SCHEMA) == 0)
    {
        link = _zn_new_tcp_link(s_addr, port);
    }
    else if (strcmp(protocol, UDP_SCHEMA) == 0)
    {
        link = _zn_new_udp_link(s_addr, port, tout);
    }

    if (link->o_func(link, tout) < 0)
        link = NULL;

    free(l);
    return link;
}

void _zn_close_link(_zn_link_t *link)
{
    link->c_func(link);
    free(link->endpoint);
    free(link);
}
