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
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/utils.h"

void _z_transport_peer_common_clear(_z_transport_peer_common_t *src) {
#if Z_FEATURE_FRAGMENTATION == 1
    _z_wbuf_clear(&src->_dbuf_reliable);
    _z_wbuf_clear(&src->_dbuf_best_effort);
#endif
    src->_remote_zid = _z_id_empty();
    _z_resource_slist_free(&src->_remote_resources);
}
void _z_transport_peer_common_copy(_z_transport_peer_common_t *dst, const _z_transport_peer_common_t *src) {
#if Z_FEATURE_FRAGMENTATION == 1
    dst->_state_reliable = src->_state_reliable;
    dst->_state_best_effort = src->_state_best_effort;
    _z_wbuf_copy(&dst->_dbuf_reliable, &src->_dbuf_reliable);
    _z_wbuf_copy(&dst->_dbuf_best_effort, &src->_dbuf_best_effort);
    dst->_patch = src->_patch;
#endif
    dst->_received = src->_received;
    dst->_remote_zid = src->_remote_zid;
}

bool _z_transport_peer_common_eq(const _z_transport_peer_common_t *left, const _z_transport_peer_common_t *right) {
    return _z_id_eq(&left->_remote_zid, &right->_remote_zid);
}

void _z_transport_peer_multicast_clear(_z_transport_peer_multicast_t *src) {
    _z_slice_clear(&src->_remote_addr);
    _z_transport_peer_common_clear(&src->common);
}

void _z_transport_peer_multicast_copy(_z_transport_peer_multicast_t *dst, const _z_transport_peer_multicast_t *src) {
    dst->_sn_res = src->_sn_res;
    _z_conduit_sn_list_copy(&dst->_sn_rx_sns, &src->_sn_rx_sns);
    dst->_lease = src->_lease;
    dst->_next_lease = src->_next_lease;
    _z_slice_copy(&dst->_remote_addr, &src->_remote_addr);
    _z_transport_peer_common_copy(&dst->common, &src->common);
}

size_t _z_transport_peer_multicast_size(const _z_transport_peer_multicast_t *src) {
    _ZP_UNUSED(src);
    return sizeof(_z_transport_peer_multicast_t);
}

bool _z_transport_peer_multicast_eq(const _z_transport_peer_multicast_t *left,
                                    const _z_transport_peer_multicast_t *right) {
    return _z_transport_peer_common_eq(&left->common, &right->common);
}

void _z_transport_peer_unicast_clear(_z_transport_peer_unicast_t *src) {
    _z_zbuf_clear(&src->flow_buff);
    _z_socket_close(&src->_socket);
    _z_transport_peer_common_clear(&src->common);
}

void _z_transport_peer_unicast_copy(_z_transport_peer_unicast_t *dst, const _z_transport_peer_unicast_t *src) {
    dst->_sn_rx_reliable = src->_sn_rx_reliable;
    dst->_sn_rx_best_effort = src->_sn_rx_best_effort;
    dst->_socket = src->_socket;
    _z_transport_peer_common_copy(&dst->common, &src->common);
}

size_t _z_transport_peer_unicast_size(const _z_transport_peer_unicast_t *src) {
    _ZP_UNUSED(src);
    return sizeof(_z_transport_peer_unicast_t);
}

bool _z_transport_peer_unicast_eq(const _z_transport_peer_unicast_t *left, const _z_transport_peer_unicast_t *right) {
    return _z_transport_peer_common_eq(&left->common, &right->common);
}

z_result_t _z_transport_peer_unicast_add(_z_transport_unicast_t *ztu, _z_transport_unicast_establish_param_t *param,
                                         _z_sys_net_socket_t socket, _z_transport_peer_unicast_t **output_peer) {
    _z_transport_peer_mutex_lock(&ztu->_common);
    // Create peer
    ztu->_peers = _z_transport_peer_unicast_slist_push_empty(ztu->_peers);
    if (ztu->_peers == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    // Fill peer data
    _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(ztu->_peers);
    peer->flow_state = _Z_FLOW_STATE_INACTIVE;
    peer->flow_curr_size = 0;
    peer->flow_buff = _z_zbuf_null();
    peer->_pending = false;
    peer->_socket = socket;
    _z_zint_t initial_sn_rx = _z_sn_decrement(ztu->_common._sn_res, param->_initial_sn_rx);
    peer->_sn_rx_reliable = initial_sn_rx;
    peer->_sn_rx_best_effort = initial_sn_rx;

    peer->common._remote_zid = param->_remote_zid;
    peer->common._received = true;
    peer->common._remote_resources = NULL;
#if Z_FEATURE_FRAGMENTATION == 1
    peer->common._patch = param->_patch < _Z_CURRENT_PATCH ? param->_patch : _Z_CURRENT_PATCH;
    peer->common._state_reliable = _Z_DBUF_STATE_NULL;
    peer->common._state_best_effort = _Z_DBUF_STATE_NULL;
    peer->common._dbuf_reliable = _z_wbuf_null();
    peer->common._dbuf_best_effort = _z_wbuf_null();
#endif
    _z_transport_peer_mutex_unlock(&ztu->_common);

    if (output_peer != NULL) {
        *output_peer = peer;
    }
    return _Z_RES_OK;
}
