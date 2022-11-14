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

#ifndef ZENOH_PICO_TRANSPORT_LINK_RX_H
#define ZENOH_PICO_TRANSPORT_LINK_RX_H

#include "zenoh-pico/link/link.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/transport/transport.h"

/*------------------ Transmission and Reception helpers ------------------*/
int8_t _z_unicast_recv_t_msg(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg);
int8_t _z_multicast_recv_t_msg(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg, _z_bytes_t *addr);

int8_t _z_link_recv_t_msg(_z_transport_message_t *t_msg, const _z_link_t *zl);

int8_t _z_unicast_recv_t_msg_na(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg);
int8_t _z_multicast_recv_t_msg_na(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg, _z_bytes_t *addr);

int8_t _z_unicast_handle_transport_message(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg);
int8_t _z_multicast_handle_transport_message(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                             _z_bytes_t *addr);

#endif /* ZENOH_PICO_TRANSPORT_LINK_RX_H */