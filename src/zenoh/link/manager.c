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

_zn_link_p_result_t _zn_open_link(const char *locator, clock_t tout)
{
    _zn_link_p_result_t r;
    r.tag = _z_res_t_OK;

    _zn_endpoint_t *endpoint = _zn_endpoint_from_string(locator);
    if (endpoint == NULL)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        goto EXIT_OPEN_LINK;
    }

    // Create transport link
    _zn_link_t *link = NULL;
    if (strcmp(endpoint->protocol, TCP_SCHEMA) == 0)
    {
        link = _zn_new_link_unicast_tcp(endpoint);
    }
    else if (strcmp(endpoint->protocol, UDP_SCHEMA) == 0)
    {
        link = _zn_new_link_unicast_udp(endpoint);
    }
    else
    {
        _zn_endpoint_free(&endpoint);

        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        goto EXIT_OPEN_LINK;
    }

    // Open transport link for communication
    _zn_socket_result_t r_sock = link->open_f(link, tout);
    if (r_sock.tag == _z_res_t_ERR)
    {
        _zn_link_free(&link);

        r.tag = _z_res_t_ERR;
        r.value.error = r_sock.value.error;
        goto EXIT_OPEN_LINK;
    }

    link->sock = r_sock.value.socket;
    r.value.link = link;

EXIT_OPEN_LINK:
    return r;
}

void _zn_link_free(_zn_link_t **zn)
{
    _zn_link_t *ptr = *zn;

    ptr->close_f(ptr);
    ptr->release_f(ptr);
    _zn_endpoint_free(&ptr->endpoint);

    *zn = NULL;
}
