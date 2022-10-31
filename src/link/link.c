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

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/utils/logging.h"

_z_link_p_result_t _z_open_link(const char *locator) {
    _z_link_p_result_t r;
    r._tag = _Z_RES_OK;
    r._value = NULL;

    _z_endpoint_result_t ep_res = _z_endpoint_from_str(locator);
    _ASSURE_RESULT(ep_res, r, _Z_ERR_INVALID_LOCATOR)
    _z_endpoint_t endpoint = ep_res._value;

    // TODO[peer]: when peer unicast mode is supported, this must be revisited
    // Create transport link
#if Z_LINK_TCP == 1
    if (_z_str_eq(endpoint._locator._protocol, TCP_SCHEMA) == true) {
        r._value = _z_new_link_tcp(endpoint);
    } else
#endif
#if Z_LINK_UDP_UNICAST == 1
        if (_z_str_eq(endpoint._locator._protocol, UDP_SCHEMA) == true) {
        r._value = _z_new_link_udp_unicast(endpoint);
    } else
#endif
#if Z_LINK_BLUETOOTH == 1
        if (_z_str_eq(endpoint._locator._protocol, BT_SCHEMA) == true) {
        r._value = _z_new_link_bt(endpoint);
    } else
#endif
#if Z_LINK_SERIAL == 1
        if (_z_str_eq(endpoint._locator._protocol, SERIAL_SCHEMA) == true) {
        r._value = _z_new_link_serial(endpoint);
    } else
#endif
    {
        r._tag = _Z_ERR_LOCATOR_INVALID;
        _z_endpoint_clear(&endpoint);
    }

    if (r._tag == _Z_RES_OK) {
        // Open transport link for communication
        if (r._value->_open_f(r._value) != _Z_RES_OK) {
            r._tag = _Z_ERR_TRANSPORT_OPEN_FAILED;
            _z_link_free(&r._value);
        }
    }

    return r;
}

_z_link_p_result_t _z_listen_link(const char *locator) {
    _z_link_p_result_t r;
    r._tag = _Z_RES_OK;
    r._value = NULL;

    _z_endpoint_result_t ep_res = _z_endpoint_from_str(locator);
    _ASSURE_RESULT(ep_res, r, _Z_ERR_INVALID_LOCATOR)
    _z_endpoint_t endpoint = ep_res._value;

    // TODO[peer]: when peer unicast mode is supported, this must be revisited
    // Create transport link
#if Z_LINK_UDP_MULTICAST == 1
    if (_z_str_eq(endpoint._locator._protocol, UDP_SCHEMA) == true) {
        r._value = _z_new_link_udp_multicast(endpoint);
    } else
#endif
#if Z_LINK_BLUETOOTH == 1
        if (_z_str_eq(endpoint._locator._protocol, BT_SCHEMA) == true) {
        r._value = _z_new_link_bt(endpoint);
    } else
#endif
    {
        r._tag = _Z_ERR_LOCATOR_INVALID;
        _z_endpoint_clear(&endpoint);
    }

    if (r._tag == _Z_RES_OK) {
        // Open transport link for listening
        if (r._value->_listen_f(r._value) != _Z_RES_OK) {
            r._tag = _Z_ERR_TRANSPORT_OPEN_FAILED;
            _z_link_free(&r._value);
        }
    }

    return r;
}

void _z_link_free(_z_link_t **zn) {
    _z_link_t *ptr = *zn;

    if (ptr != NULL) {
        ptr->_close_f(ptr);
        ptr->_free_f(ptr);
        _z_endpoint_clear(&ptr->_endpoint);

        z_free(ptr);
        *zn = NULL;
    }
}

size_t _z_link_recv_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, _z_bytes_t *addr) {
    size_t rb = link->_read_f(link, _z_zbuf_get_wptr(zbf), _z_zbuf_space_left(zbf), addr);
    if (rb != SIZE_MAX) {
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    }
    return rb;
}

size_t _z_link_recv_exact_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, size_t len, _z_bytes_t *addr) {
    size_t rb = link->_read_exact_f(link, _z_zbuf_get_wptr(zbf), len, addr);
    if (rb != SIZE_MAX) {
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    }
    return rb;
}

int8_t _z_link_send_wbuf(const _z_link_t *link, const _z_wbuf_t *wbf) {
    int8_t ret = _Z_RES_OK;

    for (size_t i = 0; (i < _z_wbuf_len_iosli(wbf)) || (ret == -1); i++) {
        _z_bytes_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        size_t n = bs.len;
        do {
            _Z_DEBUG("Sending wbuf on socket...");
            size_t wb = link->_write_f(link, bs.start, n);
            _Z_DEBUG(" sent %lu bytes\n", wb);
            if (wb == SIZE_MAX) {
                _Z_DEBUG("Error while sending data over socket [%lu]\n", wb);
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
                break;
            }
            n = n - wb;
            bs.start = bs.start + (bs.len - n);
        } while (n > (size_t)0);
    }

    return ret;
}
