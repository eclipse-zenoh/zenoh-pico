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
#include "zenoh-pico/link/config/raweth.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/utils/logging.h"

int8_t _z_open_link(_z_link_t *zl, const char *locator) {
    int8_t ret = _Z_RES_OK;

    _z_endpoint_t ep;
    ret = _z_endpoint_from_str(&ep, locator);
    if (ret == _Z_RES_OK) {
        // TODO[peer]: when peer unicast mode is supported, this must be revisited
        // Create transport link
#if Z_FEATURE_LINK_TCP == 1
        if (_z_endpoint_tcp_valid(&ep) == _Z_RES_OK) {
            ret = _z_new_link_tcp(zl, &ep);
        } else
#endif
#if Z_FEATURE_LINK_UDP_UNICAST == 1
            if (_z_endpoint_udp_unicast_valid(&ep) == _Z_RES_OK) {
            ret = _z_new_link_udp_unicast(zl, ep);
        } else
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
            if (_z_endpoint_bt_valid(&ep) == _Z_RES_OK) {
            ret = _z_new_link_bt(zl, ep);
        } else
#endif
#if Z_FEATURE_LINK_SERIAL == 1
            if (_z_endpoint_serial_valid(&ep) == _Z_RES_OK) {
            ret = _z_new_link_serial(zl, ep);
        } else
#endif
#if Z_FEATURE_LINK_WS == 1
            if (_z_endpoint_ws_valid(&ep) == _Z_RES_OK) {
            ret = _z_new_link_ws(zl, &ep);
        } else
#endif
        {
            ret = _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
        }
        if (ret == _Z_RES_OK) {
            // Open transport link for communication
            if (zl->_open_f(zl) != _Z_RES_OK) {
                ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
                _z_link_clear(zl);
            }
        } else {
            _z_endpoint_clear(&ep);
        }
    } else {
        _z_endpoint_clear(&ep);
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return ret;
}

int8_t _z_listen_link(_z_link_t *zl, const char *locator) {
    int8_t ret = _Z_RES_OK;

    _z_endpoint_t ep;
    ret = _z_endpoint_from_str(&ep, locator);
    if (ret == _Z_RES_OK) {
        // TODO[peer]: when peer unicast mode is supported, this must be revisited
        // Create transport link
#if Z_FEATURE_LINK_UDP_MULTICAST == 1
        if (_z_endpoint_udp_multicast_valid(&ep) == _Z_RES_OK) {
            ret = _z_new_link_udp_multicast(zl, ep);
        } else
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
            if (_z_endpoint_bt_valid(&ep) == _Z_RES_OK) {
            ret = _z_new_link_bt(zl, ep);
        } else
#endif
            if (_z_endpoint_raweth_valid(&ep) == _Z_RES_OK) {
            ret = _z_new_link_raweth(zl, ep);
        } else {
            ret = _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
        }
        if (ret == _Z_RES_OK) {
            // Open transport link for listening
            if (zl->_listen_f(zl) != _Z_RES_OK) {
                ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
                _z_link_clear(zl);
            }
        } else {
            _z_endpoint_clear(&ep);
        }
    } else {
        _z_endpoint_clear(&ep);
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return ret;
}

void _z_link_clear(_z_link_t *l) {
    if (l->_close_f != NULL) {
        l->_close_f(l);
    }
    if (l->_free_f != NULL) {
        l->_free_f(l);
    }
    _z_endpoint_clear(&l->_endpoint);
}

void _z_link_free(_z_link_t **l) {
    _z_link_t *ptr = *l;

    if (ptr != NULL) {
        _z_link_clear(ptr);

        z_free(ptr);
        *l = NULL;
    }
}

size_t _z_link_recv_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, _z_slice_t *addr) {
    size_t rb = link->_read_f(link, _z_zbuf_get_wptr(zbf), _z_zbuf_space_left(zbf), addr);
    if (rb != SIZE_MAX) {
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    }
    return rb;
}

size_t _z_link_recv_exact_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, size_t len, _z_slice_t *addr) {
    size_t rb = link->_read_exact_f(link, _z_zbuf_get_wptr(zbf), len, addr);
    if (rb != SIZE_MAX) {
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    }
    return rb;
}

int8_t _z_link_send_wbuf(const _z_link_t *link, const _z_wbuf_t *wbf) {
    int8_t ret = _Z_RES_OK;
    _Bool link_is_streamed = false;

    switch (link->_cap._flow) {
        case Z_LINK_CAP_FLOW_STREAM:
            link_is_streamed = true;
            break;
        case Z_LINK_CAP_FLOW_DATAGRAM:
        default:
            link_is_streamed = false;
            break;
    }
    for (size_t i = 0; (i < _z_wbuf_len_iosli(wbf)) && (ret == _Z_RES_OK); i++) {
        _z_slice_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        size_t n = bs.len;
        do {
            size_t wb = link->_write_f(link, bs.start, n);
            if (wb == SIZE_MAX) {
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
                break;
            }
            if (link_is_streamed && wb != n) {
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
                break;
            }
            n = n - wb;
            bs.start = bs.start + (bs.len - n);
        } while (n > (size_t)0);
    }

    return ret;
}
