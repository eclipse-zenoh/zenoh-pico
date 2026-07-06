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

#include "zenoh-pico/transport/unicast/connectivity.h"

#include "zenoh-pico/config.h"
#include "zenoh-pico/session/interest.h"

void _zp_unicast_report_connected_event(_z_transport_unicast_t *ztu,
                                        _z_transport_peer_unicast_hset_iter_t connected_peer_iter) {
    _z_session_t *session = _z_transport_common_get_session(&ztu->_common);
    _z_interest_push_declarations_to_peer(session, connected_peer_iter);
#if Z_FEATURE_CONNECTIVITY == 1
    _z_connectivity_peer_event_data_t peer_event_data = {0};
    uint16_t mtu = 0;
    bool is_streamed = false;
    bool is_reliable = false;
    _z_transport_peer_unicast_t *connected_peer = _z_transport_peer_unicast_hset_at(&ztu->_peers, connected_peer_iter);
    _z_transport_get_link_properties(&ztu->_common, &mtu, &is_streamed, &is_reliable);
    _z_connectivity_peer_event_data_alias_from_common(&peer_event_data, &connected_peer->common);
    _z_connectivity_peer_connected(session, &peer_event_data, false, mtu, is_streamed, is_reliable);
#endif
}

void _zp_unicast_report_disconnected_event(_z_transport_unicast_t *ztu,
                                           _z_transport_peer_unicast_hset_iter_t disconnected_peer_iter) {
    _z_session_t *zs = _z_transport_common_get_session(&ztu->_common);
    _z_interest_peer_disconnected(zs, disconnected_peer_iter);
    _z_flush_remote_resources_for_peer(zs, disconnected_peer_iter);
#if Z_FEATURE_LIVELINESS == 1 && Z_FEATURE_SUBSCRIPTION == 1
    _z_liveliness_subscription_undeclare_all(zs, disconnected_peer_iter);
#endif
#if Z_FEATURE_CONNECTIVITY == 1
    _z_connectivity_peer_event_data_t data = {0};
    uint16_t mtu = 0;
    bool is_streamed = false;
    bool is_reliable = false;
    _z_transport_peer_unicast_t *disconnected_peer =
        _z_transport_peer_unicast_hset_at(&ztu->_peers, disconnected_peer_iter);
    _z_transport_get_link_properties(&ztu->_common, &mtu, &is_streamed, &is_reliable);
    _z_connectivity_peer_event_data_alias_from_common(&data, &disconnected_peer->common);
    _z_connectivity_peer_disconnected(zs, &data, false, mtu, is_streamed, is_reliable);
#endif
}

void _zp_unicast_remove_peers_by_mask(_z_transport_unicast_t *ztu, _z_peer_mask_bitset_t peers) {
    _z_peer_mask_bitset_iter_t expired_iter = _z_peer_mask_bitset_begin_true(&peers);
    _z_peer_mask_bitset_iter_t expired_end = _z_peer_mask_bitset_end(&peers);
    if (expired_iter == expired_end) {
        return;
    }
    _z_transport_peer_mutex_lock(&ztu->_common);
    for (; expired_iter != expired_end; expired_iter = _z_peer_mask_bitset_iter_next_true(&peers, expired_iter)) {
        _z_transport_peer_unicast_hset_remove_at(&ztu->_peers, (_z_transport_peer_unicast_hset_iter_t)expired_iter,
                                                 NULL, NULL);
    }
    _z_transport_peer_mutex_unlock(&ztu->_common);
}
