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

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/link.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/utils/logging.h"

_z_link_p_result_t _z_open_link(const _z_str_t locator)
{
    _z_link_p_result_t r;

    _z_endpoint_result_t ep_res = _z_endpoint_from_str(locator);
    _ASSURE_RESULT(ep_res, r, _Z_ERR_INVALID_LOCATOR)
    _z_endpoint_t endpoint = ep_res._value._endpoint;

    // Create transport link
#if Z_LINK_TCP == 1
    if (_z_str_eq(endpoint._locator._protocol, TCP_SCHEMA))
    {
        r._value._link = _z_new_link_tcp(endpoint);
    }
    else
#endif
#if Z_LINK_UDP_UNICAST == 1
    if (_z_str_eq(endpoint._locator._protocol, UDP_SCHEMA))
    {
        r._value._link = _z_new_link_udp_unicast(endpoint);
    }
    else
#endif
#if Z_LINK_BLUETOOTH == 1
    if (_z_str_eq(endpoint._locator._protocol, BT_SCHEMA))
    {
        r.value._link = _z_new_link_bt(endpoint);
    }
    else
#endif
        goto ERR2;

    // Open transport link for communication
    if (r._value._link->_open_f(r._value._link) < 0)
        goto ERR3;

    r._tag = _Z_RES_OK;
    return r;

ERR3:
    _z_link_free(&r._value._link);
    r._value._error = -1;
    goto ERR1; // _z_link_free is releasing the endpoint

ERR2:
    r._value._error = _Z_ERR_INVALID_LOCATOR;
    _z_endpoint_clear(&endpoint);

ERR1:
    r._tag = _Z_RES_ERR;
    return r;
}

_z_link_p_result_t _z_listen_link(const _z_str_t locator)
{
    _z_link_p_result_t r;

    _z_endpoint_result_t ep_res = _z_endpoint_from_str(locator);
    _ASSURE_RESULT(ep_res, r, _Z_ERR_INVALID_LOCATOR)
    _z_endpoint_t endpoint = ep_res._value._endpoint;

    // @TODO: for now listening is only supported for UDP multicast
    // Create transport link
#if Z_LINK_UDP_MULTICAST == 1
    if (_z_str_eq(endpoint._locator._protocol, UDP_SCHEMA))
    {
        r._value._link = _z_new_link_udp_multicast(endpoint);
    }
    else
#endif
#if Z_LINK_BLUETOOTH == 1
    if (_z_str_eq(endpoint._locator._protocol, BT_SCHEMA))
    {
        r._value._link = _z_new_link_bt(endpoint);
    }
    else
#endif
        goto ERR2;

    // Open transport link for listening
    if (r._value._link->_listen_f(r._value._link) < 0)
        goto ERR3;

    r._tag = _Z_RES_OK;
    return r;

ERR3:
    _z_link_free(&r._value._link);
    r._value._error = _Z_ERR_INVALID_LOCATOR;
    goto ERR1; // _z_link_free is releasing the endpoint

ERR2:
    _z_endpoint_clear(&endpoint);

ERR1:
    r._tag = _Z_RES_ERR;
    r._value._error = -1;

    return r;
}

void _z_link_free(_z_link_t **zn)
{
    _z_link_t *ptr = *zn;

    ptr->_close_f(ptr);
    ptr->_free_f(ptr);
    _z_endpoint_clear(&ptr->_endpoint);

    free(ptr);
    *zn = NULL;
}

size_t _z_link_recv_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, _z_bytes_t *addr)
{
    size_t rb = link->_read_f(link, _z_zbuf_get_wptr(zbf), _z_zbuf_space_left(zbf), addr);
    if (rb != SIZE_MAX)
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    return rb;
}

size_t _z_link_recv_exact_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, size_t len, _z_bytes_t *addr)
{
    size_t rb = link->_read_exact_f(link, _z_zbuf_get_wptr(zbf), len, addr);
    if (rb != SIZE_MAX)
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    return rb;
}

int _z_link_send_wbuf(const _z_link_t *link, const _z_wbuf_t *wbf)
{
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++)
    {
        _z_bytes_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        size_t n = bs.len;
        size_t wb;
        do
        {
            _Z_DEBUG("Sending wbuf on socket...");
            wb = link->_write_f(link, bs.val, n);
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
