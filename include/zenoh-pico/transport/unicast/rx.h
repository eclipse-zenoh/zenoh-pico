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

#ifndef ZENOH_PICO_UNICAST_RX_H
#define ZENOH_PICO_UNICAST_RX_H

#include "zenoh-pico/transport/transport.h"

int8_t _z_unicast_recv_t_msg(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg);
int8_t _z_unicast_recv_t_msg_na(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg);
int8_t _z_unicast_handle_transport_message(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg);

#endif /* ZENOH_PICO_UNICAST_RX_H */
