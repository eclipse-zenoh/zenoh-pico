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

#ifndef ZENOH_PICO_MULTICAST_TRANSPORT_H
#define ZENOH_PICO_MULTICAST_TRANSPORT_H

#include "zenoh-pico/api/types.h"

#ifdef __cplusplus
extern "C" {
#endif

z_result_t _z_multicast_transport_create(_z_transport_t *zt, _z_link_t *zl,
                                         _z_transport_multicast_establish_param_t *param);
z_result_t _z_multicast_open_peer(_z_transport_multicast_establish_param_t *param, const _z_link_t *zl,
                                  const _z_id_t *local_zid);
z_result_t _z_multicast_open_client(_z_transport_multicast_establish_param_t *param, const _z_link_t *zl,
                                    const _z_id_t *local_zid);
z_result_t _z_multicast_send_close(_z_transport_multicast_t *ztm, uint8_t reason, bool link_only);
z_result_t _z_multicast_transport_close(_z_transport_multicast_t *ztm, uint8_t reason);
void _z_multicast_transport_clear(_z_transport_multicast_t *ztm, bool detach_tasks);

#if (Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1) && Z_FEATURE_MULTI_THREAD == 1
static inline void _z_multicast_peer_mutex_lock(_z_transport_multicast_t *ztm) { _z_mutex_lock(&ztm->_mutex_peer); }
static inline void _z_multicast_peer_mutex_unlock(_z_transport_multicast_t *ztm) { _z_mutex_unlock(&ztm->_mutex_peer); }
#else
static inline void _z_multicast_peer_mutex_lock(_z_transport_multicast_t *ztm) { _ZP_UNUSED(ztm); }
static inline void _z_multicast_peer_mutex_unlock(_z_transport_multicast_t *ztm) { _ZP_UNUSED(ztm); }
#endif  // (Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1) && Z_FEATURE_MULTI_THREAD == 1

#ifdef __cplusplus
}
#endif
#endif /* ZENOH_PICO_MULTICAST_TRANSPORT_H */
