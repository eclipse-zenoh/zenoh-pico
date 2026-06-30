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

z_result_t _z_link_socket_wait_peers_readable(const _z_link_t *link, _z_link_peer_iter_t *peers, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_COMMON_SOCKET_OPS_H */
