/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef ZENOH_C_NET_MSGCODEC_H
#define ZENOH_C_NET_MSGCODEC_H

#include "zenoh/net/codec.h"
#include "zenoh/net/result.h"
#include "zenoh/net/types.h"
#include "zenoh/net/property.h"
#include "zenoh/net/private/msg.h"

/*------------------ Session Message ------------------*/
void _zn_session_message_encode(z_iobuf_t *buf, const _zn_session_message_t *m);
_zn_session_message_p_result_t z_session_message_decode(z_iobuf_t *buf);
void _zn_session_message_decode_na(z_iobuf_t *buf, _zn_session_message_p_result_t *r);

/*------------------ Zenoh Message ------------------*/
void zn_zenoh_message_encode(z_iobuf_t *buf, const zn_zenoh_message_t *m);
zn_zenoh_message_p_result_t zn_zenoh_message_decode(z_iobuf_t *buf);
void zn_zenoh_message_decode_na(z_iobuf_t *buf, zn_zenoh_message_p_result_t *r);

#endif /* ZENOH_C_NET_MSGCODEC_H */

// NOTE: the following headers are for unit tests only
#ifdef ZENOH_C_NET_MSGCODEC_T
#define _ZN_DECLARE_ENCODE(name) \
    void _zn_##name##_encode(z_iobuf_t *buf, uint8_t header, const _zn_##name##_t *m)

#define _ZN_DECLARE_ENCODE_NOH(name) \
    void _zn_##name##_encode(z_iobuf_t *buf, const _zn_##name##_t *m)

#define _ZN_DECLARE_DECODE(name)                                               \
    _zn_##name##_result_t _zn_##name##_decode(z_iobuf_t *buf, uint8_t header); \
    void _zn_##name##_decode_na(z_iobuf_t *buf, uint8_t header, _zn_##name##_result_t *r)

#define _ZN_DECLARE_DECODE_NOH(name)                           \
    _zn_##name##_result_t _zn_##name##_decode(z_iobuf_t *buf); \
    void _zn_##name##_decode_na(z_iobuf_t *buf, _zn_##name##_result_t *r)

/*------------------ Message Fields ------------------*/
void _zn_payload_encode(z_iobuf_t *buf, const z_iobuf_t *bs);
z_iobuf_t _zn_payload_decode(z_iobuf_t *buf);

void _zn_timestamp_encode(z_iobuf_t *buf, const z_timestamp_t *ts);
void _zn_timestamp_decode_na(z_iobuf_t *buf, _zn_timestamp_result_t *r);
_zn_timestamp_result_t _zn_timestamp_decode(z_iobuf_t *buf);

ZN_DECLARE_ENCODE(res_key);
ZN_DECLARE_DECODE(res_key);

/*------------------ Message Decorators ------------------*/
_ZN_DECLARE_ENCODE_NOH(attachment);
_ZN_DECLARE_DECODE(attachment);

_ZN_DECLARE_ENCODE_NOH(reply_context);
_ZN_DECLARE_DECODE(reply_context);

/*------------------ Zenoh Message ------------------*/
_ZN_DECLARE_ENCODE(res_decl);
_ZN_DECLARE_DECODE(res_decl);

_ZN_DECLARE_ENCODE(pub_decl);
_ZN_DECLARE_DECODE(pub_decl);

_ZN_DECLARE_ENCODE(sub_decl);
_ZN_DECLARE_DECODE(sub_decl);

_ZN_DECLARE_ENCODE(qle_decl);
_ZN_DECLARE_DECODE(qle_decl);

_ZN_DECLARE_ENCODE_NOH(forget_res_decl);
_ZN_DECLARE_DECODE_NOH(forget_res_decl);

_ZN_DECLARE_ENCODE(forget_pub_decl);
_ZN_DECLARE_DECODE(forget_pub_decl);

_ZN_DECLARE_ENCODE(forget_sub_decl);
_ZN_DECLARE_DECODE(forget_sub_decl);

_ZN_DECLARE_ENCODE(forget_qle_decl);
_ZN_DECLARE_DECODE(forget_qle_decl);

_ZN_DECLARE_ENCODE_NOH(declaration);
_ZN_DECLARE_DECODE_NOH(declaration);

_ZN_DECLARE_ENCODE_NOH(declare);
_ZN_DECLARE_DECODE_NOH(declare);

ZN_DECLARE_ENCODE_NOH(data_info);
ZN_DECLARE_DECODE_NOH(data_info);

_ZN_DECLARE_ENCODE(data);
_ZN_DECLARE_DECODE(data);

_ZN_DECLARE_ENCODE(pull);
_ZN_DECLARE_DECODE(pull);

_ZN_DECLARE_ENCODE(query);
_ZN_DECLARE_DECODE(query);

/*------------------ Session Message ------------------*/
_ZN_DECLARE_ENCODE(scout);
_ZN_DECLARE_DECODE(scout);

_ZN_DECLARE_ENCODE(hello);
_ZN_DECLARE_DECODE(hello);

_ZN_DECLARE_ENCODE(open);
_ZN_DECLARE_DECODE(open);

_ZN_DECLARE_ENCODE(accept);
_ZN_DECLARE_DECODE(accept);

_ZN_DECLARE_ENCODE(close);
_ZN_DECLARE_DECODE(close);

_ZN_DECLARE_ENCODE(sync);
_ZN_DECLARE_DECODE(sync);

_ZN_DECLARE_ENCODE(ack_nack);
_ZN_DECLARE_DECODE(ack_nack);

_ZN_DECLARE_ENCODE(keep_alive);
_ZN_DECLARE_DECODE(keep_alive);

_ZN_DECLARE_ENCODE_NOH(ping_pong);
_ZN_DECLARE_DECODE_NOH(ping_pong);

_ZN_DECLARE_ENCODE(frame);
_ZN_DECLARE_DECODE(frame);

#endif /* ZENOH_C_NET_MSGCODEC_T */
