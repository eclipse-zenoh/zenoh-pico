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

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/utils.h"

void _z_transport_peer_entry_clear(_z_transport_peer_entry_t *src) {
#if Z_FEATURE_FRAGMENTATION == 1
    _z_wbuf_clear(&src->_dbuf_reliable);
    _z_wbuf_clear(&src->_dbuf_best_effort);
#endif

    src->_remote_zid = _z_id_empty();
    _z_slice_clear(&src->_remote_addr);
}

void _z_transport_peer_entry_copy(_z_transport_peer_entry_t *dst, const _z_transport_peer_entry_t *src) {
#if Z_FEATURE_FRAGMENTATION == 1
    dst->_state_reliable = src->_state_reliable;
    dst->_state_best_effort = src->_state_best_effort;
    _z_wbuf_copy(&dst->_dbuf_reliable, &src->_dbuf_reliable);
    _z_wbuf_copy(&dst->_dbuf_best_effort, &src->_dbuf_best_effort);
    dst->_patch = src->_patch;
#endif

    dst->_sn_res = src->_sn_res;
    _z_conduit_sn_list_copy(&dst->_sn_rx_sns, &src->_sn_rx_sns);

    dst->_lease = src->_lease;
    dst->_next_lease = src->_next_lease;
    dst->_received = src->_received;

    dst->_remote_zid = src->_remote_zid;
    _z_slice_copy(&dst->_remote_addr, &src->_remote_addr);
}

size_t _z_transport_peer_entry_size(const _z_transport_peer_entry_t *src) {
    _ZP_UNUSED(src);
    return sizeof(_z_transport_peer_entry_t);
}

bool _z_transport_peer_entry_eq(const _z_transport_peer_entry_t *left, const _z_transport_peer_entry_t *right) {
    return _z_id_eq(&left->_remote_zid, &right->_remote_zid);
}

void _z_transport_unicast_peer_clear(_z_transport_unicast_peer_t *src) {
    _z_zbuf_clear(&src->flow_buff);
#if Z_FEATURE_FRAGMENTATION == 1
    _z_wbuf_clear(&src->_dbuf_reliable);
    _z_wbuf_clear(&src->_dbuf_best_effort);
#endif
    src->_remote_zid = _z_id_empty();
    _z_socket_close(&src->_socket);
}

void _z_transport_unicast_peer_copy(_z_transport_unicast_peer_t *dst, const _z_transport_unicast_peer_t *src) {
#if Z_FEATURE_FRAGMENTATION == 1
    dst->_state_reliable = src->_state_reliable;
    dst->_state_best_effort = src->_state_best_effort;
    _z_wbuf_copy(&dst->_dbuf_reliable, &src->_dbuf_reliable);
    _z_wbuf_copy(&dst->_dbuf_best_effort, &src->_dbuf_best_effort);
    dst->_patch = src->_patch;
#endif
    dst->_sn_rx_reliable = src->_sn_rx_reliable;
    dst->_sn_rx_best_effort = src->_sn_rx_best_effort;
    dst->_received = src->_received;
    dst->_remote_zid = src->_remote_zid;
    dst->_socket = src->_socket;
}

size_t _z_transport_unicast_peer_size(const _z_transport_unicast_peer_t *src) {
    _ZP_UNUSED(src);
    return sizeof(_z_transport_unicast_peer_t);
}

bool _z_transport_unicast_peer_eq(const _z_transport_unicast_peer_t *left, const _z_transport_unicast_peer_t *right) {
    return _z_id_eq(&left->_remote_zid, &right->_remote_zid);
}

z_result_t _z_transport_unicast_peer_add(_z_transport_unicast_t *ztu, _z_transport_unicast_establish_param_t *param,
                                         _z_sys_net_socket_t socket) {
    // Create peer
    _z_transport_unicast_peer_t *peer = (_z_transport_unicast_peer_t *)z_malloc(sizeof(_z_transport_unicast_peer_t));
    if (peer == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Fill peer data
    peer->flow_state = _Z_FLOW_STATE_INACTIVE;
    peer->flow_curr_size = 0;
    peer->flow_buff = _z_zbuf_null();
    peer->_pending = false;
    peer->_socket = socket;
    peer->_remote_zid = param->_remote_zid;
    _z_zint_t initial_sn_rx = _z_sn_decrement(ztu->_common._sn_res, param->_initial_sn_rx);
    peer->_sn_rx_reliable = initial_sn_rx;
    peer->_sn_rx_best_effort = initial_sn_rx;
    peer->_received = true;
#if Z_FEATURE_FRAGMENTATION == 1
    peer->_patch = param->_patch < _Z_CURRENT_PATCH ? param->_patch : _Z_CURRENT_PATCH;
    peer->_state_reliable = _Z_DBUF_STATE_NULL;
    peer->_state_best_effort = _Z_DBUF_STATE_NULL;
    peer->_dbuf_reliable = _z_wbuf_null();
    peer->_dbuf_best_effort = _z_wbuf_null();
#endif
    // Insert peer
    _z_transport_peer_mutex_lock(&ztu->_common);
    ztu->_peers = _z_transport_unicast_peer_list_push(ztu->_peers, peer);
    _z_transport_peer_mutex_unlock(&ztu->_common);
    if (ztu->_peers == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}
