//
// Copyright (c) 2026 ZettaScale Technology
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

#include "zenoh-pico/link/common/socket_ops.h"

#include <stdint.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/utils/logging.h"

typedef struct _z_link_socket_peer_impl_t {
    _z_link_peer_impl_t _base;
    _z_sys_net_socket_t _socket;
    _z_link_socket_close_f _close_f;
} _z_link_socket_peer_impl_t;

static _z_link_socket_peer_impl_t *_z_link_socket_peer_impl(_z_link_peer_t *peer) {
    return (_z_link_socket_peer_impl_t *)_z_link_peer_impl(peer);
}

static const _z_link_socket_peer_impl_t *_z_link_socket_peer_impl_const(const _z_link_peer_t *peer) {
    return (const _z_link_socket_peer_impl_t *)_z_link_peer_impl_const(peer);
}

_z_sys_net_socket_t *_z_link_socket_peer_get_socket(_z_link_peer_t *peer) {
    _z_link_socket_peer_impl_t *impl = _z_link_socket_peer_impl(peer);
    return impl == NULL ? NULL : &impl->_socket;
}

const _z_sys_net_socket_t *_z_link_socket_peer_get_socket_const(const _z_link_peer_t *peer) {
    const _z_link_socket_peer_impl_t *impl = _z_link_socket_peer_impl_const(peer);
    return impl == NULL ? NULL : &impl->_socket;
}

static void _z_link_socket_peer_impl_clear(_z_link_peer_impl_t *base) {
    _z_link_socket_peer_impl_t *impl = (_z_link_socket_peer_impl_t *)base;
    if (impl == NULL) {
        return;
    }
    if (impl->_close_f != NULL) {
        impl->_close_f(&impl->_socket);
        impl->_close_f = NULL;
    }
}

z_result_t _z_link_socket_peer_from_socket(_z_link_peer_t *peer, _z_sys_net_socket_t socket,
                                           _z_link_socket_close_f close_f, const _z_link_peer_ops_t *ops) {
    if ((peer == NULL) || (ops == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    _z_link_socket_peer_impl_t *impl = (_z_link_socket_peer_impl_t *)z_malloc(sizeof(_z_link_socket_peer_impl_t));
    if (impl == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    _z_link_peer_impl_init(&impl->_base, ops, _z_link_socket_peer_impl_clear);
    impl->_socket = socket;
    impl->_close_f = close_f;

    z_result_t ret = _z_link_peer_init(peer, &impl->_base);
    if (ret != _Z_RES_OK) {
        _z_link_socket_peer_impl_clear(&impl->_base);
        z_free(impl);
        return ret;
    }
    return _Z_RES_OK;
}

void _z_link_socket_peer_close(_z_link_peer_t *peer) {
    _z_link_socket_peer_impl_t *impl = _z_link_socket_peer_impl(peer);
    if ((impl == NULL) || (impl->_close_f == NULL)) {
        return;
    }
    _z_link_socket_close_f close_f = impl->_close_f;
    impl->_close_f = NULL;
    close_f(&impl->_socket);
}

static void _z_link_peer_socket_iter_reset(_z_socket_wait_iter_t *iter) {
    _z_link_peer_iter_reset((_z_link_peer_iter_t *)iter->_ctx);
}

static bool _z_link_peer_socket_iter_next(_z_socket_wait_iter_t *iter) {
    return _z_link_peer_iter_next((_z_link_peer_iter_t *)iter->_ctx);
}

static const _z_sys_net_socket_t *_z_link_peer_socket_iter_get_socket(const _z_socket_wait_iter_t *iter) {
    const _z_link_peer_t *peer = _z_link_peer_iter_get_peer((const _z_link_peer_iter_t *)iter->_ctx);
    if (peer == NULL) {
        return NULL;
    }
    return _z_link_socket_peer_get_socket_const(peer);
}

static void _z_link_peer_socket_iter_set_ready(_z_socket_wait_iter_t *iter, bool ready) {
    _z_link_peer_iter_set_ready((_z_link_peer_iter_t *)iter->_ctx, ready);
}

z_result_t _z_link_socket_wait_peers_readable(const _z_link_t *link, _z_link_peer_iter_t *peers, uint32_t timeout_ms) {
    _ZP_UNUSED(link);
    _z_socket_wait_iter_t socket_iter = {
        ._ctx = peers,
        ._current_entry = NULL,
        ._reset = _z_link_peer_socket_iter_reset,
        ._next = _z_link_peer_socket_iter_next,
        ._get_socket = _z_link_peer_socket_iter_get_socket,
        ._set_ready = _z_link_peer_socket_iter_set_ready,
    };
    return _z_socket_wait_readable(&socket_iter, timeout_ms);
}

z_result_t _z_link_socket_peer_set_blocking(const _z_link_peer_t *peer, bool blocking) {
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    if (socket == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    return _z_socket_set_blocking(socket, blocking);
}

z_result_t _z_link_socket_peer_get_endpoints(const _z_link_peer_t *peer, char *local, size_t local_len, char *remote,
                                             size_t remote_len) {
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    if (socket == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    return _z_socket_get_endpoints(socket, local, local_len, remote, remote_len);
}
