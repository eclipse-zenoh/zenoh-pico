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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_TRANSPORT_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_TRANSPORT_H

#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/protocol/iobuf.h"
int8_t _z_scouting_message_encode(_z_wbuf_t *buf, const _z_scouting_message_t *msg);
int8_t _z_scouting_message_decode(_z_scouting_message_t *msg, _z_zbuf_t *buf);
int8_t _z_scouting_message_decode_na(_z_scouting_message_t *msg, _z_zbuf_t *buf);

int8_t _z_transport_message_encode(_z_wbuf_t *buf, const _z_transport_message_t *msg);
int8_t _z_transport_message_decode(_z_transport_message_t *msg, _z_zbuf_t *buf);

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_TRANSPORT_H */
