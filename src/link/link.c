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

#include "zenoh-pico/link/driver_registry.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/utils/logging.h"

typedef enum {
    _Z_LINK_OPERATION_OPEN,
    _Z_LINK_OPERATION_LISTEN,
} _z_link_operation_t;

static inline _z_link_driver_activate_f _z_link_driver_operation_f(const _z_link_driver_t *driver,
                                                                   _z_link_operation_t operation) {
    switch (operation) {
        case _Z_LINK_OPERATION_OPEN:
            return driver->_open_f;
        case _Z_LINK_OPERATION_LISTEN:
            return driver->_listen_f;
    }
    return NULL;
}

static z_result_t _z_link_create_and_activate(_z_link_t **zl, const _z_string_t *locator,
                                              const _z_config_t *session_cfg, _z_link_operation_t operation) {
    z_result_t ret = _Z_RES_OK;

    if (zl == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    *zl = NULL;

    _z_endpoint_t ep;
    ret = _z_endpoint_from_string(&ep, locator);
    if (ret != _Z_RES_OK) {
        _z_endpoint_clear(&ep);
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    const _z_link_driver_t *const *drivers = _z_link_driver_registry();
    // The registry is NULL terminated.
    for (size_t i = 0; drivers[i] != NULL; i++) {
        const _z_link_driver_t *driver = drivers[i];
        _z_link_driver_activate_f activate_f = _z_link_driver_operation_f(driver, operation);

        if (activate_f == NULL) {
            continue;
        }
        ret = driver->_validate_f(&ep);
        if (ret != _Z_RES_OK) {
            _Z_DEBUG("Link driver %zu rejected locator for %s: %d", i,
                     operation == _Z_LINK_OPERATION_OPEN ? "open" : "listen", (int)ret);
            continue;
        }

        _z_link_t *link = NULL;
        ret = driver->_create_f(&link, &ep, session_cfg);
        if (ret != _Z_RES_OK) {
            _z_endpoint_clear(&ep);
            return ret;
        }

        if (link == NULL) {
            _Z_ERROR("Link driver create returned OK without creating a link");
            _z_endpoint_clear(&ep);
            return _Z_ERR_GENERIC;
        }

        if (activate_f(link) != _Z_RES_OK) {
            _Z_ERROR_LOG(_Z_ERR_TRANSPORT_OPEN_FAILED);
            _z_link_free(&link);
            return _Z_ERR_TRANSPORT_OPEN_FAILED;
        }
        *zl = link;
        return _Z_RES_OK;
    }

    _z_endpoint_clear(&ep);
    _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN);
    return _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
}

z_result_t _z_open_link(_z_link_t **zl, const _z_string_t *locator, const _z_config_t *session_cfg) {
    return _z_link_create_and_activate(zl, locator, session_cfg, _Z_LINK_OPERATION_OPEN);
}

z_result_t _z_listen_link(_z_link_t **zl, const _z_string_t *locator, const _z_config_t *session_cfg) {
    return _z_link_create_and_activate(zl, locator, session_cfg, _Z_LINK_OPERATION_LISTEN);
}

void _z_link_clear(_z_link_t *l) {
    if (l == NULL) {
        return;
    }

    /*
     * Close while driver state and the default peer are still available; then release
     * the default peer before dropping embedded driver-private state.
     */
    if (l->_close_f != NULL) {
        l->_close_f(l);
    }

    _z_link_peer_clear(&l->_peer);
    _z_endpoint_clear(&l->_endpoint);

    if (l->_drop_f != NULL) {
        l->_drop_f(l);
    }

    *l = (_z_link_t){0};
}

void _z_link_free(_z_link_t **l) {
    _z_link_t *ptr = *l;

    if (ptr != NULL) {
        _z_link_clear(ptr);

        z_free(ptr);
        *l = NULL;
    }
}

void _z_link_peer_impl_clear(_z_link_peer_impl_t *impl) {
    if ((impl != NULL) && (impl->_clear_f != NULL)) {
        impl->_clear_f(impl);
    }
}

_z_link_peer_impl_t *_z_link_peer_impl(_z_link_peer_t *peer) {
    return (peer == NULL) || (peer->_impl._cnt == NULL) ? NULL : peer->_impl._val;
}

const _z_link_peer_impl_t *_z_link_peer_impl_const(const _z_link_peer_t *peer) {
    return (peer == NULL) || (peer->_impl._cnt == NULL) ? NULL : peer->_impl._val;
}

bool _z_link_peer_check(const _z_link_peer_t *peer) { return _z_link_peer_impl_const(peer) != NULL; }

void _z_link_peer_impl_init(_z_link_peer_impl_t *impl, const _z_link_peer_ops_t *ops,
                            _z_link_peer_impl_clear_f clear_f) {
    if (impl == NULL) {
        return;
    }
    impl->_ops = ops;
    impl->_clear_f = clear_f;
}

z_result_t _z_link_peer_init(_z_link_peer_t *peer, _z_link_peer_impl_t *impl) {
    if ((peer == NULL) || (impl == NULL) || (impl->_ops == NULL) || (impl->_clear_f == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    _z_link_peer_impl_rc_t rc = _z_link_peer_impl_rc_new(impl);
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

void _z_link_peer_close(_z_link_peer_t *peer) {
    const _z_link_peer_impl_t *impl = _z_link_peer_impl_const(peer);
    if ((impl != NULL) && (impl->_ops != NULL) && (impl->_ops->_close_f != NULL)) {
        impl->_ops->_close_f(peer);
    }
}

void _z_link_peer_clear(_z_link_peer_t *peer) {
    if (peer != NULL) {
        if (peer->_impl._cnt == NULL) {
            return;
        }
        _z_link_peer_impl_rc_t impl = peer->_impl;
        peer->_impl = _z_link_peer_impl_rc_null();
        _z_link_peer_impl_rc_drop(&impl);
    }
}

size_t _z_link_peer_read(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len) {
    const _z_link_peer_impl_t *impl = _z_link_peer_impl_const(peer);
    const _z_link_peer_ops_t *ops = impl != NULL ? impl->_ops : NULL;
    if ((ops == NULL) || (ops->_read_f == NULL)) {
        return SIZE_MAX;
    }
    return ops->_read_f(link, peer, ptr, len);
}

size_t _z_link_peer_write(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr, size_t len) {
    const _z_link_peer_impl_t *impl = _z_link_peer_impl_const(peer);
    const _z_link_peer_ops_t *ops = impl != NULL ? impl->_ops : NULL;
    if ((ops == NULL) || (ops->_write_f == NULL)) {
        return SIZE_MAX;
    }
    return ops->_write_f(link, peer, ptr, len);
}

z_result_t _z_link_peer_set_blocking(const _z_link_peer_t *peer, bool blocking) {
    const _z_link_peer_impl_t *impl = _z_link_peer_impl_const(peer);
    const _z_link_peer_ops_t *ops = impl != NULL ? impl->_ops : NULL;
    if ((ops == NULL) || (ops->_set_blocking_f == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    return ops->_set_blocking_f(peer, blocking);
}

z_result_t _z_link_peer_get_endpoints(const _z_link_peer_t *peer, char *local, size_t local_len, char *remote,
                                      size_t remote_len) {
    const _z_link_peer_impl_t *impl = _z_link_peer_impl_const(peer);
    const _z_link_peer_ops_t *ops = impl != NULL ? impl->_ops : NULL;
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
    _Z_CLEAN_RETURN_IF_ERR(zl->_open_peer_f(zl, peer, locator, session_cfg), _z_link_peer_clear(peer));
    return _Z_RES_OK;
}

z_result_t _z_link_peer_from_default(const _z_link_t *zl, _z_link_peer_t *peer) {
    if ((zl == NULL) || (peer == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    *peer = _z_link_peer_clone(&zl->_peer);
    return _z_link_peer_check(peer) ? _Z_RES_OK : _Z_ERR_GENERIC;
}

z_result_t _z_link_peer_from_link(const _z_link_t *zl, _z_link_peer_t *peer) {
    if ((zl == NULL) || (peer == NULL) || (zl->_peer_from_link_f == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    *peer = _z_link_peer_null();
    _Z_CLEAN_RETURN_IF_ERR(zl->_peer_from_link_f(zl, peer), _z_link_peer_clear(peer));
    return _Z_RES_OK;
}

bool _z_link_can_accept_peers(const _z_link_t *zl) { return (zl != NULL) && (zl->_accept_peer_f != NULL); }

z_result_t _z_link_accept_peer(const _z_link_t *zl, _z_link_peer_t *peer) {
    if ((zl == NULL) || (peer == NULL) || (zl->_accept_peer_f == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    *peer = _z_link_peer_null();
    _Z_CLEAN_RETURN_IF_ERR(zl->_accept_peer_f(zl, peer), _z_link_peer_clear(peer));
    return _Z_RES_OK;
}

z_result_t _z_link_accept_peer_complete(const _z_link_t *zl, _z_link_peer_t *peer) {
    if ((zl == NULL) || (peer == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    if (zl->_accept_peer_complete_f == NULL) {
        return _Z_RES_OK;
    }
    return zl->_accept_peer_complete_f(zl, peer);
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
    uint8_t *ptr = _z_zbuf_get_wptr(zbf);
    size_t total = 0;
    while (total < len) {
        size_t rb = _z_link_peer_read(link, peer, &ptr[total], len - total);
        if (rb == SIZE_MAX) {
            return SIZE_MAX;
        }
        if (rb == 0) {
            return 0;
        }
        total += rb;
    }

    _z_zbuf_set_wpos(zbf, _z_zbuf_get_wpos(zbf) + len);
    return len;
}

z_result_t _z_link_send_wbuf(const _z_link_t *link, const _z_wbuf_t *wbf) {
    z_result_t ret = _Z_RES_OK;
    bool link_is_streamed = link->_cap._flow == Z_LINK_CAP_FLOW_STREAM;

    for (size_t i = 0; (i < _z_wbuf_len_iosli(wbf)) && (ret == _Z_RES_OK); i++) {
        _z_slice_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        size_t n = bs.len;
        while (n > (size_t)0) {
            size_t wb = link->_write_f(link, bs.start, n);
            if ((wb == SIZE_MAX) || (wb == 0) || (wb > n)) {
                _Z_ERROR_LOG(_Z_ERR_TRANSPORT_TX_FAILED);
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
                break;
            }
            // Stream links may complete a frame across multiple writes; datagram links must send it atomically.
            if (!link_is_streamed && wb != n) {
                _Z_ERROR_LOG(_Z_ERR_TRANSPORT_TX_FAILED);
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
                break;
            }
            n = n - wb;
            bs.start = bs.start + wb;
        }
    }

    return ret;
}

z_result_t _z_link_peer_send_wbuf(const _z_link_t *link, const _z_wbuf_t *wbf, const _z_link_peer_t *peer) {
    z_result_t ret = _Z_RES_OK;
    bool link_is_streamed = link->_cap._flow == Z_LINK_CAP_FLOW_STREAM;

    for (size_t i = 0; (i < _z_wbuf_len_iosli(wbf)) && (ret == _Z_RES_OK); i++) {
        _z_slice_t bs = _z_iosli_to_bytes(_z_wbuf_get_iosli(wbf, i));
        size_t n = bs.len;
        while (n > (size_t)0) {
            size_t wb = _z_link_peer_write(link, peer, bs.start, n);
            // Stream links may complete a frame across multiple writes; datagram links must send it atomically.
            if ((wb == SIZE_MAX) || (wb == 0) || (wb > n) || (!link_is_streamed && wb != n)) {
                _Z_ERROR_LOG(_Z_ERR_TRANSPORT_TX_FAILED);
                ret = _Z_ERR_TRANSPORT_TX_FAILED;
                break;
            }
            n = n - wb;
            bs.start = bs.start + wb;
        }
    }

    return ret;
}
