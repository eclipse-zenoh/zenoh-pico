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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_MESSAGE_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_MESSAGE_H

#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/iobuf.h"

int8_t _z_push_body_encode(_z_wbuf_t *wbf, const _z_push_body_t *pshb);
int8_t _z_push_body_decode(_z_push_body_t *body, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_query_encode(_z_wbuf_t *wbf, const _z_msg_query_t *query);
int8_t _z_query_decode(_z_msg_query_t *query, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_reply_encode(_z_wbuf_t *wbf, const _z_msg_reply_t *reply);
int8_t _z_reply_decode(_z_msg_reply_t *reply, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_err_encode(_z_wbuf_t *wbf, const _z_msg_err_t *err);
int8_t _z_err_decode(_z_msg_err_t *err, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_put_encode(_z_wbuf_t *wbf, const _z_msg_put_t *put);
int8_t _z_put_decode(_z_msg_put_t *put, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_del_encode(_z_wbuf_t *wbf, const _z_msg_del_t *del);
int8_t _z_del_decode(_z_msg_del_t *del, _z_zbuf_t *zbf, uint8_t header);

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_MESSAGE_H */
