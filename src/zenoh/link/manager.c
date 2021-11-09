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

    // Create endpoint from locator
    _zn_endpoint_t *endpoint = _zn_endpoint_from_string(locator);
    if (endpoint == NULL)
    {
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        goto _ZN_OPEN_LINK_ERROR_1;
    }

    // Create transport link
    if (strcmp(endpoint->protocol, TCP_SCHEMA) == 0)
    {
        r.value.link = _zn_new_link_tcp(endpoint);
    }
    else if (strcmp(endpoint->protocol, UDP_SCHEMA) == 0)
    {
        r.value.link = _zn_new_link_udp_unicast(endpoint);
    }
    else
    {
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        goto _ZN_OPEN_LINK_ERROR_2;
    }

    // Open transport link for communication
    _zn_socket_result_t r_sock = r.value.link->open_f(r.value.link, tout);
    if (r_sock.tag == _z_res_t_ERR)
    {
        r.value.error = r_sock.value.error;
        goto _ZN_OPEN_LINK_ERROR_3;
    }

    return r;

_ZN_OPEN_LINK_ERROR_3:
    _zn_link_free(&r.value.link);
    goto _ZN_OPEN_LINK_ERROR_1; // _zn_link_free is releasing the endpoint

_ZN_OPEN_LINK_ERROR_2:
    _zn_endpoint_free(&endpoint);

_ZN_OPEN_LINK_ERROR_1:
    r.tag = _z_res_t_ERR;
    return r;
}

_zn_link_p_result_t _zn_listen_link(const char *locator, clock_t tout)
{
    _zn_link_p_result_t r;
    r.tag = _z_res_t_OK;

    _zn_endpoint_t *endpoint = _zn_endpoint_from_string(locator);
    if (endpoint == NULL)
    {
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        goto _ZN_LISTEN_LINK_ERROR_1;
    }

    // TODO: for now listening is only supported for UDP multicast
    // Create transport link
    if (strcmp(endpoint->protocol, UDP_SCHEMA) == 0)
    {
        r.value.link = _zn_new_link_udp_multicast(endpoint);
    }
    else
    {
        r.value.error = _zn_err_t_INVALID_LOCATOR;
        goto _ZN_LISTEN_LINK_ERROR_2;
    }

    // Open transport link for listening
    _zn_socket_result_t r_sock = r.value.link->listen_f(r.value.link, tout);
    if (r_sock.tag == _z_res_t_ERR)
    {
        r.value.error = r_sock.value.error;
        goto _ZN_LISTEN_LINK_ERROR_3;
    }

    return r;

_ZN_LISTEN_LINK_ERROR_3:
    _zn_link_free(&r.value.link);
    goto _ZN_LISTEN_LINK_ERROR_1; // _zn_link_free is releasing the endpoint

_ZN_LISTEN_LINK_ERROR_2:
    _zn_endpoint_free(&endpoint);

_ZN_LISTEN_LINK_ERROR_1:
    r.tag = _z_res_t_ERR;
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
