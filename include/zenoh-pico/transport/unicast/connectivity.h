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
#ifndef INCLUDE_ZENOH_PICO_TRANSPORT_UNICAST_CONNECTIVITY_H
#define INCLUDE_ZENOH_PICO_TRANSPORT_UNICAST_CONNECTIVITY_H

#include "zenoh-pico/transport/unicast/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

void _zp_unicast_report_connected_event(_z_transport_unicast_t *ztu,
                                        _z_transport_peer_unicast_hset_iter_t connected_peer_iter);
void _zp_unicast_report_disconnected_event(_z_transport_unicast_t *ztu,
                                           _z_transport_peer_unicast_hset_iter_t disconnected_peer_iter);
void _zp_unicast_remove_peers_by_mask(_z_transport_unicast_t *ztu, _z_peer_mask_bitset_t peers);

#ifdef __cplusplus
}
#endif
#endif  // INCLUDE_ZENOH_PICO_TRANSPORT_UNICAST_CONNECTIVITY_H
