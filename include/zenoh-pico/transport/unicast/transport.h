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

#ifndef ZENOH_PICO_UNICAST_TRANSPORT_H
#define ZENOH_PICO_UNICAST_TRANSPORT_H

#include "zenoh-pico/api/types.h"

z_result_t _z_unicast_transport_create(_z_transport_t *zt, _z_link_t *zl,
                                       _z_transport_unicast_establish_param_t *param);
z_result_t _z_unicast_open_client(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                  const _z_id_t *local_zid);
z_result_t _z_unicast_open_peer(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                const _z_id_t *local_zid);
z_result_t _z_unicast_send_close(_z_transport_unicast_t *ztu, uint8_t reason, bool link_only);
z_result_t _z_unicast_transport_close(_z_transport_unicast_t *ztu, uint8_t reason);
void _z_unicast_transport_clear(_z_transport_t *zt);

#if Z_FEATURE_UNICAST_TRANSPORT == 1 && Z_FEATURE_MULTI_THREAD == 1
static inline z_result_t _z_unicast_tx_mutex_lock(_z_transport_unicast_t *ztu, bool block) {
    if (block) {
        _z_mutex_lock(&ztu->_mutex_tx);
        return _Z_RES_OK;
    } else {
        return _z_mutex_try_lock(&ztu->_mutex_tx);
    }
}
static inline void _z_unicast_tx_mutex_unlock(_z_transport_unicast_t *ztu) { _z_mutex_unlock(&ztu->_mutex_tx); }
static inline void _z_unicast_rx_mutex_lock(_z_transport_unicast_t *ztu) { _z_mutex_lock(&ztu->_mutex_rx); }
static inline void _z_unicast_rx_mutex_unlock(_z_transport_unicast_t *ztu) { _z_mutex_unlock(&ztu->_mutex_rx); }

#else
static inline z_result_t _z_unicast_tx_mutex_lock(_z_transport_unicast_t *ztu, bool block) {
    _ZP_UNUSED(ztu);
    _ZP_UNUSED(block);
    return _Z_RES_OK;
}
static inline void _z_unicast_tx_mutex_unlock(_z_transport_unicast_t *ztu) { _ZP_UNUSED(ztu); }
static inline void _z_unicast_rx_mutex_lock(_z_transport_unicast_t *ztu) { _ZP_UNUSED(ztu); }
static inline void _z_unicast_rx_mutex_unlock(_z_transport_unicast_t *ztu) { _ZP_UNUSED(ztu); }
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1 && Z_FEATURE_MULTI_THREAD == 1
#endif  /* ZENOH_PICO_UNICAST_TRANSPORT_H */
