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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_NETWORK_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_NETWORK_H

#include <stdint.h>

#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/iobuf.h"
int8_t _z_push_encode(_z_wbuf_t *wbf, const _z_n_msg_push_t *msg);
int8_t _z_push_decode(_z_n_msg_push_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_request_encode(_z_wbuf_t *wbf, const _z_n_msg_request_t *msg);
int8_t _z_request_decode(_z_n_msg_request_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_response_encode(_z_wbuf_t *wbf, const _z_n_msg_response_t *msg);
int8_t _z_response_decode(_z_n_msg_response_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_response_final_encode(_z_wbuf_t *wbf, const _z_n_msg_response_final_t *msg);
int8_t _z_response_final_decode(_z_n_msg_response_final_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_declare_encode(_z_wbuf_t *wbf, const _z_n_msg_declare_t *decl);
int8_t _z_declare_decode(_z_n_msg_declare_t *decl, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_network_message_encode(_z_wbuf_t *wbf, const _z_network_message_t *msg);
int8_t _z_network_message_decode(_z_network_message_t *msg, _z_zbuf_t *zbf);

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_NETWORK_H */
