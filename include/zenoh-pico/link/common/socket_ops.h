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

#ifndef ZENOH_PICO_LINK_COMMON_SOCKET_OPS_H
#define ZENOH_PICO_LINK_COMMON_SOCKET_OPS_H

#include "zenoh-pico/link/link.h"

#ifdef __cplusplus
extern "C" {
#endif

const _z_link_peer_ops_t *_z_link_peer_socket_ops(void);
z_result_t _z_link_socket_wait_peers_readable(const _z_link_t *link, _z_link_peer_iter_t *peers, uint32_t timeout_ms);
z_result_t _z_link_socket_open_peer(const _z_link_t *link, _z_link_peer_t *peer, const _z_string_t *locator,
                                    const _z_config_t *session_cfg);
z_result_t _z_link_socket_peer_from_link(const _z_link_t *link, _z_link_peer_t *peer);

static inline _z_link_peer_t _z_link_peer_from_socket(_z_sys_net_socket_t socket, bool owns_socket) {
    _z_link_peer_t peer = _z_link_peer_null();
    peer._ops = _z_link_peer_socket_ops();
    peer._socket = socket;
    peer._owns_socket = owns_socket;
    return peer;
}

static inline _z_sys_net_socket_t *_z_link_peer_get_socket(_z_link_peer_t *peer) { return &peer->_socket; }

static inline const _z_sys_net_socket_t *_z_link_peer_get_socket_const(const _z_link_peer_t *peer) {
    return &peer->_socket;
}

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_COMMON_SOCKET_OPS_H */
