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

#ifndef ZENOH_PICO_TRANSPORT_TX_H
#define ZENOH_PICO_TRANSPORT_TX_H

#include "zenoh-pico/link/link.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/transport/transport.h"

void __unsafe_z_prepare_wbuf(_z_wbuf_t *buf, uint8_t link_flow_capability);
void __unsafe_z_finalize_wbuf(_z_wbuf_t *buf, uint8_t link_flow_capability);
/*This function is unsafe because it operates in potentially concurrent
        data.*Make sure that the following mutexes are locked before calling this function : *-ztu->mutex_tx */
int8_t __unsafe_z_serialize_zenoh_fragment(_z_wbuf_t *dst, _z_wbuf_t *src, z_reliability_t reliability, size_t sn);

/*------------------ Transmission and Reception helpers ------------------*/
int8_t _z_send_t_msg(_z_transport_t *zt, const _z_transport_message_t *t_msg);
int8_t _z_link_send_t_msg(const _z_link_t *zl, const _z_transport_message_t *t_msg);

#endif /* ZENOH_PICO_TRANSPORT_TX_H */
