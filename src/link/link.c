//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include "zenoh-pico/link/link.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/utils/logging.h"

_zn_link_p_result_t _zn_open_link(const z_str_t locator)
{
    _zn_link_p_result_t r;

    _zn_endpoint_result_t ep_res = _zn_endpoint_from_str(locator);
    _ASSURE_RESULT(ep_res, r, _zn_err_t_INVALID_LOCATOR)
    _zn_endpoint_t endpoint = ep_res.value.endpoint;

    // Create transport link
    if (_z_str_eq(endpoint.locator.protocol, TCP_SCHEMA))
    {
        r.value.link = _zn_new_link_tcp(endpoint);
    }
    else if (_z_str_eq(endpoint.locator.protocol, UDP_SCHEMA))
    {
        r.value.link = _zn_new_link_udp_unicast(endpoint);
    }
    else
        goto ERR2;

    // Open transport link for communication
    if (r.value.link->open_f(r.value.link) < 0)
        goto ERR3;

    r.tag = _z_res_t_OK;
    return r;

ERR3:
    _zn_link_free(&r.value.link);
    r.value.error = -1;
    goto ERR1; // _zn_link_free is releasing the endpoint

ERR2:
    r.value.error = _zn_err_t_INVALID_LOCATOR;
    _zn_endpoint_clear(&endpoint);

ERR1:
    r.tag = _z_res_t_ERR;
    return r;
}

_zn_link_p_result_t _zn_listen_link(const z_str_t locator)
{
    _zn_link_p_result_t r;

    _zn_endpoint_result_t ep_res = _zn_endpoint_from_str(locator);
    _ASSURE_RESULT(ep_res, r, _zn_err_t_INVALID_LOCATOR)
    _zn_endpoint_t endpoint = ep_res.value.endpoint;

    // @TODO: for now listening is only supported for UDP multicast
    // Create transport link
    if (_z_str_eq(endpoint.locator.protocol, UDP_SCHEMA))
    {
        r.value.link = _zn_new_link_udp_multicast(endpoint);
    }
    else
        goto ERR2;

    // Open transport link for listening
    if (r.value.link->listen_f(r.value.link) < 0)
        goto ERR3;

    r.tag = _z_res_t_OK;
    return r;

ERR3:
    _zn_link_free(&r.value.link);
    r.value.error = _zn_err_t_INVALID_LOCATOR;
    goto ERR1; // _zn_link_free is releasing the endpoint

ERR2:
    _zn_endpoint_clear(&endpoint);

ERR1:
    r.tag = _z_res_t_ERR;
    r.value.error = -1;

    return r;
}

void _zn_link_free(_zn_link_t **zn)
{
    _zn_link_t *ptr = *zn;

    ptr->close_f(ptr);
    ptr->free_f(ptr);
    _zn_endpoint_clear(&ptr->endpoint);

    free(ptr);
    *zn = NULL;
}

size_t _zn_link_recv_zbuf(const _zn_link_t *link, _z_zbuf_t *zbf, z_bytes_t *addr)
{
    size_t rb = link->read_f(link, _z_zbuf_get_wptr(zbf), _z_zbuf_space_left(zbf), addr);
    if (rb != SIZE_MAX)
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    return rb;
}

size_t _zn_link_recv_exact_zbuf(const _zn_link_t *link, _z_zbuf_t *zbf, size_t len, z_bytes_t *addr)
{
    size_t rb = link->read_exact_f(link, _z_zbuf_get_wptr(zbf), len, addr);
    if (rb != SIZE_MAX)
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    return rb;
}

int _zn_link_send_wbuf(const _zn_link_t *link, const _z_wbuf_t *wbf)
{
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++)
    {
        z_bytes_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        size_t n = bs.len;
        size_t wb;
        do
        {
            _Z_DEBUG("Sending wbuf on socket...");
            wb = link->write_f(link, bs.val, n);
            _Z_DEBUG(" sent %d bytes\n", wb);
            if (wb == SIZE_MAX)
            {
                _Z_DEBUG("Error while sending data over socket [%d]\n", wb);
                return -1;
            }
            n -= wb;
            bs.val += bs.len - n;
        } while (n > 0);
    }

    return 0;
}
