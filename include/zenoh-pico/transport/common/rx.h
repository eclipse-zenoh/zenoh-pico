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

#ifndef ZENOH_PICO_TRANSPORT_RX_H
#define ZENOH_PICO_TRANSPORT_RX_H

#include "zenoh-pico/link/link.h"
#include "zenoh-pico/transport/transport.h"

/*------------------ Transmission and Reception helpers ------------------*/
size_t _z_read_stream_size(_z_zbuf_t *zbuf);
int8_t _z_link_recv_t_msg(_z_transport_message_t *t_msg, const _z_link_t *zl);

#endif /* ZENOH_PICO_TRANSPORT_RX_H */
