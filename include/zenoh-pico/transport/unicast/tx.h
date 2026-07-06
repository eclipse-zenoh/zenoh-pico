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
//

#ifndef ZENOH_PICO_UNICAST_TX_H
#define ZENOH_PICO_UNICAST_TX_H

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/transport/transport.h"

#ifdef __cplusplus
extern "C" {
#endif
z_result_t _z_transport_unicast_send_t_msg(_z_transport_unicast_t *ztu, const _z_transport_message_t *t_msg,
                                           const _z_peer_mask_bitset_t *opt_peers);
z_result_t _z_transport_unicast_send_n_msg(_z_transport_unicast_t *ztu, const _z_network_message_t *n_msg,
                                           z_reliability_t reliability, z_congestion_control_t cong_ctrl,
                                           const _z_peer_mask_bitset_t *opt_peers);
#if Z_FEATURE_BATCHING == 1
z_result_t _z_transport_unicast_send_n_batch(_z_transport_unicast_t *ztu);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_UNICAST_TX_H */
