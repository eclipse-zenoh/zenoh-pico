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
#include "zenoh-pico/transport/multicast/connectivity.h"

#include "zenoh-pico/config.h"

void _zp_multicast_report_disconnected_event(
    _z_transport_multicast_t *ztm, _z_address_to_transport_peer_multicast_hmap_iter_t disconnected_peer_iter) {
#if Z_FEATURE_CONNECTIVITY == 1
    uint16_t mtu = 0;
    bool is_streamed = false;
    bool is_reliable = false;
    _z_transport_get_link_properties(&ztm->_common, &mtu, &is_streamed, &is_reliable);
#endif
    _z_transport_peer_multicast_t *dropped_peer =
        &_z_address_to_transport_peer_multicast_hmap_at(&ztm->_peers, disconnected_peer_iter)->val;
    _z_session_t *s = _z_transport_common_get_session(&ztm->_common);
    _z_interest_peer_disconnected(s, &dropped_peer->common);
#if Z_FEATURE_CONNECTIVITY == 1
    _z_connectivity_peer_event_data_t disconnected_peer = {0};
    _z_connectivity_peer_event_data_alias_from_common(&disconnected_peer, &dropped_peer->common);
    _z_connectivity_peer_disconnected(s, &disconnected_peer, true, mtu, is_streamed, is_reliable);
#endif
}

void _zp_multicast_report_connected_event(_z_transport_multicast_t *ztm,
                                          _z_address_to_transport_peer_multicast_hmap_iter_t connected_peer_iter) {
#if Z_FEATURE_CONNECTIVITY == 1
    _z_transport_peer_multicast_t *connected_peer =
        &_z_address_to_transport_peer_multicast_hmap_at(&ztm->_peers, connected_peer_iter)->val;
    _z_connectivity_peer_event_data_t connected_peer_data = {0};
    uint16_t mtu = 0;
    bool is_streamed = false;
    bool is_reliable = false;
    _z_transport_get_link_properties(&ztm->_common, &mtu, &is_streamed, &is_reliable);
    _z_connectivity_peer_event_data_alias_from_common(&connected_peer_data, &connected_peer->common);
    _z_connectivity_peer_connected(_z_transport_common_get_session(&ztm->_common), &connected_peer_data, true, mtu,
                                   is_streamed, is_reliable);
    _z_connectivity_peer_event_data_clear(&connected_peer_data);
#endif
    // TODO: notify session about new connected peer
    (void)ztm;
    (void)connected_peer_iter;
}

void _zp_multicast_remove_peer_by_iter(_z_transport_multicast_t *ztm,
                                       _z_address_to_transport_peer_multicast_hmap_iter_t iter) {
    _zp_multicast_report_disconnected_event(ztm, iter);
    _z_transport_peer_mutex_lock(&ztm->_common);
    _z_address_to_transport_peer_multicast_hmap_remove_at(&ztm->_peers, iter, NULL, NULL);
    _z_transport_peer_mutex_unlock(&ztm->_common);
}
