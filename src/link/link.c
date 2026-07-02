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

z_result_t _z_open_link(_z_link_t *zl, const _z_string_t *locator, const _z_config_t *session_cfg) {
#if Z_FEATURE_LINK_TLS != 1
    _ZP_UNUSED(session_cfg);
#endif
    z_result_t ret = _Z_RES_OK;

    _z_endpoint_t ep;
    ret = _z_endpoint_from_string(&ep, locator);
    if (ret == _Z_RES_OK) {
        // Create transport link
        ret = _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
        if (_z_endpoint_tcp_matches(&ep)) {
            ret = _z_new_link_tcp(zl, &ep);
        }
#if Z_FEATURE_LINK_UDP_UNICAST == 1
        else if (_z_endpoint_udp_unicast_matches(&ep)) {
            ret = _z_new_link_udp_unicast(zl, ep);
        }
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
        else if (_z_endpoint_bt_matches(&ep)) {
            ret = _z_new_link_bt(zl, ep);
        }
#endif
#if Z_FEATURE_LINK_SERIAL == 1
        else if (_z_endpoint_serial_matches(&ep)) {
            ret = _z_new_link_serial(zl, ep);
        }
#endif
#if Z_FEATURE_LINK_WS == 1
        else if (_z_endpoint_ws_matches(&ep)) {
            ret = _z_new_link_ws(zl, &ep);
        }
#endif
#if Z_FEATURE_LINK_TLS == 1
        else if (_z_endpoint_tls_matches(&ep)) {
            ret = _z_new_link_tls(zl, &ep, session_cfg);
        }
#endif
        else {
            _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN);
            ret = _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
        }
        if (ret == _Z_RES_OK) {
            // Open transport link for communication
            if (zl->_open_f(zl) != _Z_RES_OK) {
                _Z_ERROR_LOG(_Z_ERR_TRANSPORT_OPEN_FAILED);
                ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
                _z_link_clear(zl);
            }
        } else {
            _z_endpoint_clear(&ep);
        }
    } else {
        _z_endpoint_clear(&ep);
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return ret;
}

z_result_t _z_listen_link(_z_link_t *zl, const _z_string_t *locator, const _z_config_t *session_cfg) {
#if Z_FEATURE_LINK_TLS != 1
    _ZP_UNUSED(session_cfg);
#endif
    z_result_t ret = _Z_RES_OK;

    _z_endpoint_t ep;
    ret = _z_endpoint_from_string(&ep, locator);
    if (ret == _Z_RES_OK) {
        // Create transport link
        ret = _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
        if (_z_endpoint_tcp_matches(&ep)) {
            ret = _z_new_link_tcp(zl, &ep);
        }
#if Z_FEATURE_LINK_TLS == 1
        else if (_z_endpoint_tls_matches(&ep)) {
            ret = _z_new_link_tls(zl, &ep, session_cfg);
        }
#endif
#if Z_FEATURE_LINK_UDP_MULTICAST == 1
        else if (_z_endpoint_udp_multicast_matches(&ep)) {
            ret = _z_new_link_udp_multicast(zl, ep);
        }
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
        else if (_z_endpoint_bt_matches(&ep)) {
            ret = _z_new_link_bt(zl, ep);
        }
#endif
#if Z_FEATURE_RAWETH_TRANSPORT == 1
        else if (_z_endpoint_raweth_matches(&ep)) {
            ret = _z_new_link_raweth(zl, ep);
        }
#endif
        else {
            _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN);
            ret = _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
        }
        if (ret == _Z_RES_OK) {
            // Open transport link for listening
            if (zl->_listen_f(zl) != _Z_RES_OK) {
                _Z_ERROR_LOG(_Z_ERR_TRANSPORT_OPEN_FAILED);
                ret = _Z_ERR_TRANSPORT_OPEN_FAILED;
                _z_link_clear(zl);
            }
        } else {
            _z_endpoint_clear(&ep);
        }
    } else {
        _z_endpoint_clear(&ep);
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return ret;
}

void _z_link_clear(_z_link_t *l) {
    if (l == NULL) {
        return;
    }
    if (l->_close_f != NULL) {
        l->_close_f(l);
    }
    _z_link_peer_clear(&l->_peer);
    l->_peer = _z_link_peer_null();
    if (l->_free_f != NULL) {
        l->_free_f(l);
    }
    _z_endpoint_clear(&l->_endpoint);
    if (l->_drop_f != NULL) {
        l->_drop_f(l->_state);
    }
    l->_state = NULL;
    l->_drop_f = NULL;
}

void _z_link_free(_z_link_t **l) {
    _z_link_t *ptr = *l;

    if (ptr != NULL) {
        _z_link_clear(ptr);

        z_free(ptr);
        *l = NULL;
    }
}

void *_z_link_state(_z_link_t *zl) { return zl == NULL ? NULL : zl->_state; }

const void *_z_link_state_const(const _z_link_t *zl) { return zl == NULL ? NULL : zl->_state; }

void _z_link_peer_impl_clear(_z_link_peer_impl_t *impl) {
    if (impl == NULL) {
        return;
    }
    if (impl->_drop_f != NULL) {
        impl->_drop_f(impl->_state);
    }
    *impl = (_z_link_peer_impl_t){0};
}

bool _z_link_peer_check(const _z_link_peer_t *peer) {
    return (peer != NULL) && (peer->_impl._cnt != NULL) && (peer->_impl._val != NULL);
}

z_result_t _z_link_peer_init(_z_link_peer_t *peer, const _z_link_peer_ops_t *ops, void *state,
                             _z_link_peer_drop_f drop_f) {
    if ((peer == NULL) || (ops == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    _z_link_peer_impl_t impl = {
        ._ops = ops,
        ._state = state,
        ._drop_f = drop_f,
    };
    _z_link_peer_impl_rc_t rc = _z_link_peer_impl_rc_new_from_val(&impl);
    if (rc._cnt == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    *peer = (_z_link_peer_t){._impl = rc};
    return _Z_RES_OK;
}

_z_link_peer_t _z_link_peer_clone(const _z_link_peer_t *peer) {
    _z_link_peer_t clone = _z_link_peer_null();
    if (_z_link_peer_check(peer)) {
        clone._impl = _z_link_peer_impl_rc_clone(&peer->_impl);
    }
    return clone;
}

void *_z_link_peer_state(_z_link_peer_t *peer) { return _z_link_peer_check(peer) ? peer->_impl._val->_state : NULL; }

const void *_z_link_peer_state_const(const _z_link_peer_t *peer) {
    return _z_link_peer_check(peer) ? peer->_impl._val->_state : NULL;
}

void _z_link_peer_close(_z_link_peer_t *peer) {
    if (_z_link_peer_check(peer) && (peer->_impl._val->_ops != NULL) && (peer->_impl._val->_ops->_close_f != NULL)) {
        peer->_impl._val->_ops->_close_f(peer);
    }
}

void _z_link_peer_clear(_z_link_peer_t *peer) {
    if (peer != NULL) {
        _z_link_peer_impl_rc_drop(&peer->_impl);
    }
}

size_t _z_link_peer_read(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len) {
    const _z_link_peer_ops_t *ops = _z_link_peer_check(peer) ? peer->_impl._val->_ops : NULL;
    if ((ops == NULL) || (ops->_read_f == NULL)) {
        return SIZE_MAX;
    }
    return ops->_read_f(link, peer, ptr, len);
}

size_t _z_link_peer_write(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr, size_t len) {
    const _z_link_peer_ops_t *ops = _z_link_peer_check(peer) ? peer->_impl._val->_ops : NULL;
    if ((ops == NULL) || (ops->_write_f == NULL)) {
        return SIZE_MAX;
    }
    return ops->_write_f(link, peer, ptr, len);
}

z_result_t _z_link_peer_set_blocking(const _z_link_peer_t *peer, bool blocking) {
    const _z_link_peer_ops_t *ops = _z_link_peer_check(peer) ? peer->_impl._val->_ops : NULL;
    if ((ops == NULL) || (ops->_set_blocking_f == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    return ops->_set_blocking_f(peer, blocking);
}

z_result_t _z_link_peer_get_endpoints(const _z_link_peer_t *peer, char *local, size_t local_len, char *remote,
                                      size_t remote_len) {
    const _z_link_peer_ops_t *ops = _z_link_peer_check(peer) ? peer->_impl._val->_ops : NULL;
    if ((ops == NULL) || (ops->_get_endpoints_f == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    return ops->_get_endpoints_f(peer, local, local_len, remote, remote_len);
}

z_result_t _z_link_wait_peers_readable(const _z_link_t *link, _z_link_peer_iter_t *peers, uint32_t timeout_ms) {
    if ((link == NULL) || (peers == NULL) || (link->_wait_peers_readable_f == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    return link->_wait_peers_readable_f(link, peers, timeout_ms);
}

z_result_t _z_link_open_peer(const _z_link_t *zl, _z_link_peer_t *peer, const _z_string_t *locator,
                             const _z_config_t *session_cfg) {
    if ((zl == NULL) || (peer == NULL) || (zl->_open_peer_f == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    *peer = _z_link_peer_null();
    z_result_t ret = zl->_open_peer_f(zl, peer, locator, session_cfg);
    if (ret != _Z_RES_OK) {
        _z_link_peer_clear(peer);
    }
    return ret;
}

z_result_t _z_link_peer_from_link(const _z_link_t *zl, _z_link_peer_t *peer) {
    if ((zl == NULL) || (peer == NULL) || (zl->_peer_from_link_f == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    *peer = _z_link_peer_null();
    z_result_t ret = zl->_peer_from_link_f(zl, peer);
    if (ret != _Z_RES_OK) {
        _z_link_peer_clear(peer);
    }
    return ret;
}

bool _z_link_can_accept_peers(const _z_link_t *zl) { return (zl != NULL) && (zl->_accept_peer_f != NULL); }

z_result_t _z_link_accept_peer(const _z_link_t *zl, _z_link_peer_t *peer) {
    if ((zl == NULL) || (peer == NULL) || (zl->_accept_peer_f == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    *peer = _z_link_peer_null();
    z_result_t ret = zl->_accept_peer_f(zl, peer);
    if (ret != _Z_RES_OK) {
        _z_link_peer_clear(peer);
    }
    return ret;
}

size_t _z_link_recv_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, _z_slice_t *addr) {
    size_t rb = link->_read_f(link, _z_zbuf_get_wptr(zbf), _z_zbuf_writable_space_left(zbf), addr);
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

size_t _z_link_peer_recv_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, const _z_link_peer_t *peer) {
    size_t rb = _z_link_peer_read(link, peer, _z_zbuf_get_wptr(zbf), _z_zbuf_writable_space_left(zbf));
    if (rb != SIZE_MAX) {
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    }
    return rb;
}

size_t _z_link_peer_recv_exact_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, size_t len, const _z_link_peer_t *peer) {
    if (zbf == NULL) {
        return SIZE_MAX;
    }
    size_t total = 0;
    while (total < len) {
        size_t rb = _z_link_peer_read(link, peer, _z_zbuf_get_wptr(zbf), len - total);
        if (rb == SIZE_MAX) {
            return SIZE_MAX;
        }
        if (rb == 0) {
            return total;
        }
        total += rb;
        _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + rb);
    }
    return total;
}

z_result_t _z_link_send_wbuf(const _z_link_t *link, const _z_wbuf_t *wbf) {
    z_result_t ret = _Z_RES_OK;
    bool link_is_streamed = link->_cap._flow == Z_LINK_CAP_FLOW_STREAM;

    for (size_t i = 0; (i < _z_wbuf_len_iosli(wbf)) && (ret == _Z_RES_OK); i++) {
        _z_slice_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        size_t n = bs.len;
        do {
            size_t wb = link->_write_f(link, bs.start, n);
            if ((wb == SIZE_MAX) || (wb > n)) {
                _Z_ERROR_LOG(_Z_ERR_TRANSPORT_TX_FAILED);
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
                break;
            }
            if (link_is_streamed && wb != n) {
                _Z_ERROR_LOG(_Z_ERR_TRANSPORT_TX_FAILED);
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
                break;
            }
            n = n - wb;
            bs.start = bs.start + (bs.len - n);
        } while (n > (size_t)0);
    }

    return ret;
}

z_result_t _z_link_peer_send_wbuf(const _z_link_t *link, const _z_wbuf_t *wbf, _z_link_peer_t *peer) {
    z_result_t ret = _Z_RES_OK;
    bool link_is_streamed = link->_cap._flow == Z_LINK_CAP_FLOW_STREAM;

    const _z_link_peer_ops_t *ops = _z_link_peer_check(peer) ? peer->_impl._val->_ops : NULL;
    if ((ops == NULL) || (ops->_write_f == NULL)) {
        _Z_ERROR_LOG(_Z_ERR_TRANSPORT_TX_FAILED);
        _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_TX_FAILED);
    }

    for (size_t i = 0; (i < _z_wbuf_len_iosli(wbf)) && (ret == _Z_RES_OK); i++) {
        _z_slice_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        size_t n = bs.len;
        do {
            size_t wb = ops->_write_f(link, peer, bs.start, n);
            if ((wb == SIZE_MAX) || (wb > n) || (link_is_streamed && wb != n)) {
                _Z_ERROR_LOG(_Z_ERR_TRANSPORT_TX_FAILED);
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
                break;
            }
            n = n - wb;
            bs.start = bs.start + (bs.len - n);
        } while (n > (size_t)0);
    }

    return ret;
}
