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

#ifndef ZENOH_PICO_MSGCODEC_H
#define ZENOH_PICO_MSGCODEC_H

#define _ZENOH_PICO_FRAME_MESSAGES_VEC_SIZE 32

#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Result declarations ------------------*/
_Z_RESULT_DECLARE(_z_timestamp_t, timestamp)
_Z_RESULT_DECLARE(_z_keyexpr_t, keyexpr)
_Z_RESULT_DECLARE(_z_subinfo_t, subinfo)
_Z_RESULT_DECLARE(_z_target_t, query_target)
_Z_RESULT_DECLARE(z_consolidation_mode_t, query_consolidation)

_Z_RESULT_DECLARE(_z_data_info_t, data_info)
_Z_RESULT_DECLARE(_z_payload_t, payload)
_Z_P_RESULT_DECLARE(_z_attachment_t, attachment)
_Z_P_RESULT_DECLARE(_z_reply_context_t, reply_context)

_Z_RESULT_DECLARE(_z_t_msg_scout_t, scout)
_Z_RESULT_DECLARE(_z_t_msg_hello_t, hello)
_Z_RESULT_DECLARE(_z_t_msg_join_t, join)
_Z_RESULT_DECLARE(_z_t_msg_init_t, init)
_Z_RESULT_DECLARE(_z_t_msg_open_t, open)
_Z_RESULT_DECLARE(_z_t_msg_close_t, close)
_Z_RESULT_DECLARE(_z_t_msg_sync_t, sync)
_Z_RESULT_DECLARE(_z_t_msg_ack_nack_t, ack_nack)
_Z_RESULT_DECLARE(_z_t_msg_keep_alive_t, keep_alive)
_Z_RESULT_DECLARE(_z_t_msg_ping_pong_t, ping_pong)
_Z_RESULT_DECLARE(_z_t_msg_frame_t, frame)
_Z_RESULT_DECLARE(_z_transport_message_t, transport_message)

_Z_RESULT_DECLARE(_z_res_decl_t, res_decl)
_Z_RESULT_DECLARE(_z_pub_decl_t, pub_decl)
_Z_RESULT_DECLARE(_z_sub_decl_t, sub_decl)
_Z_RESULT_DECLARE(_z_qle_decl_t, qle_decl)
_Z_RESULT_DECLARE(_z_forget_res_decl_t, forget_res_decl)
_Z_RESULT_DECLARE(_z_forget_pub_decl_t, forget_pub_decl)
_Z_RESULT_DECLARE(_z_forget_sub_decl_t, forget_sub_decl)
_Z_RESULT_DECLARE(_z_forget_qle_decl_t, forget_qle_decl)
_Z_RESULT_DECLARE(_z_declaration_t, declaration)
_Z_RESULT_DECLARE(_z_msg_declare_t, declare)
_Z_RESULT_DECLARE(_z_msg_data_t, data)
_Z_RESULT_DECLARE(_z_msg_pull_t, pull)
_Z_RESULT_DECLARE(_z_msg_query_t, query)
_Z_RESULT_DECLARE(_z_zenoh_message_t, zenoh_message)

/*------------------ Internal Zenoh-net Macros ------------------*/
#define _Z_DECLARE_ENCODE(name) int _z_##name##_encode(_z_wbuf_t *wbf, uint8_t header, const _z_##name##_t *m)

#define _Z_DECLARE_ENCODE_NOH(name) int _z_##name##_encode(_z_wbuf_t *wbf, const _z_##name##_t *m)

#define _Z_DECLARE_DECODE(name)                                              \
    _z_##name##_result_t _z_##name##_decode(_z_zbuf_t *zbf, uint8_t header); \
    void _z_##name##_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_##name##_result_t *r)

#define _Z_DECLARE_DECODE_NOH(name)                          \
    _z_##name##_result_t _z_##name##_decode(_z_zbuf_t *zbf); \
    void _z_##name##_decode_na(_z_zbuf_t *zbf, _z_##name##_result_t *r)

#define _Z_DECLARE_P_DECODE(name)                                              \
    _z_##name##_p_result_t _z_##name##_decode(_z_zbuf_t *zbf, uint8_t header); \
    void _z_##name##_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_##name##_p_result_t *r)

#define _Z_DECLARE_P_DECODE_NOH(name)                          \
    _z_##name##_p_result_t _z_##name##_decode(_z_zbuf_t *zbf); \
    void _z_##name##_decode_na(_z_zbuf_t *zbf, _z_##name##_p_result_t *r)

/*------------------ Transport Message ------------------*/
_Z_DECLARE_ENCODE_NOH(transport_message);
_Z_DECLARE_DECODE_NOH(transport_message);

/*------------------ Zenoh Message ------------------*/
_Z_DECLARE_ENCODE_NOH(zenoh_message);
_Z_DECLARE_DECODE_NOH(zenoh_message);

#endif /* ZENOH_PICO_MSGCODEC_H */

// NOTE: the following headers are for unit testing only
#ifdef ZENOH_PICO_TEST_H
// /*------------------ Message Fields ------------------*/
_Z_DECLARE_ENCODE_NOH(payload);
_Z_DECLARE_DECODE_NOH(payload);

_Z_DECLARE_ENCODE_NOH(timestamp);
_Z_DECLARE_DECODE_NOH(timestamp);

_Z_DECLARE_ENCODE_NOH(subinfo);
_Z_DECLARE_DECODE(subinfo);

_Z_DECLARE_ENCODE(keyexpr);
_Z_DECLARE_DECODE(keyexpr);

_Z_DECLARE_ENCODE_NOH(data_info);
_Z_DECLARE_DECODE_NOH(data_info);

// /*------------------ Message Decorators ------------------*/
_Z_DECLARE_ENCODE_NOH(attachment);
_Z_DECLARE_P_DECODE(attachment);

_Z_DECLARE_ENCODE_NOH(reply_context);
_Z_DECLARE_P_DECODE(reply_context);

// /*------------------ Zenoh Message ------------------*/
_Z_DECLARE_ENCODE(res_decl);
_Z_DECLARE_DECODE(res_decl);

_Z_DECLARE_ENCODE(pub_decl);
_Z_DECLARE_DECODE(pub_decl);

_Z_DECLARE_ENCODE(sub_decl);
_Z_DECLARE_DECODE(sub_decl);

_Z_DECLARE_ENCODE(qle_decl);
_Z_DECLARE_DECODE(qle_decl);

_Z_DECLARE_ENCODE_NOH(forget_res_decl);
_Z_DECLARE_DECODE_NOH(forget_res_decl);

_Z_DECLARE_ENCODE(forget_pub_decl);
_Z_DECLARE_DECODE(forget_pub_decl);

_Z_DECLARE_ENCODE(forget_sub_decl);
_Z_DECLARE_DECODE(forget_sub_decl);

_Z_DECLARE_ENCODE(forget_qle_decl);
_Z_DECLARE_DECODE(forget_qle_decl);

_Z_DECLARE_ENCODE_NOH(declaration);
_Z_DECLARE_DECODE_NOH(declaration);

int _z_declare_encode(_z_wbuf_t *wbf, const _z_msg_declare_t *msg);
void _z_declare_decode_na(_z_zbuf_t *zbf, _z_declare_result_t *r);
_z_declare_result_t _z_declare_decode(_z_zbuf_t *zbf);

int _z_data_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_data_t *msg);
void _z_data_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_data_result_t *r);
_z_data_result_t _z_data_decode(_z_zbuf_t *zbf, uint8_t header);

int _z_pull_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_pull_t *msg);
void _z_pull_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_pull_result_t *r);
_z_pull_result_t _z_pull_decode(_z_zbuf_t *zbf, uint8_t header);

int _z_query_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_query_t *msg);
void _z_query_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_query_result_t *r);
_z_query_result_t _z_query_decode(_z_zbuf_t *zbf, uint8_t header);

// /*------------------ Transport Message ------------------*/
int _z_join_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_join_t *msg);
void _z_join_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_join_result_t *r);
_z_join_result_t _z_join_decode(_z_zbuf_t *zbf, uint8_t header);

int _z_init_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_init_t *msg);
void _z_init_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_init_result_t *r);
_z_init_result_t _z_init_decode(_z_zbuf_t *zbf, uint8_t header);

int _z_open_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_open_t *msg);
void _z_open_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_open_result_t *r);
_z_open_result_t _z_open_decode(_z_zbuf_t *zbf, uint8_t header);

int _z_close_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_close_t *msg);
void _z_close_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_close_result_t *r);
_z_close_result_t _z_close_decode(_z_zbuf_t *zbf, uint8_t header);

int _z_sync_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_sync_t *msg);
void _z_sync_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_sync_result_t *r);
_z_sync_result_t _z_sync_decode(_z_zbuf_t *zbf, uint8_t header);

int _z_ack_nack_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_ack_nack_t *msg);
void _z_ack_nack_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_ack_nack_result_t *r);
_z_ack_nack_result_t _z_ack_nack_decode(_z_zbuf_t *zbf, uint8_t header);

int _z_keep_alive_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_keep_alive_t *msg);
void _z_keep_alive_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_keep_alive_result_t *r);
_z_keep_alive_result_t _z_keep_alive_decode(_z_zbuf_t *zbf, uint8_t header);

int _z_ping_pong_encode(_z_wbuf_t *wbf, const _z_t_msg_ping_pong_t *msg);
void _z_ping_pong_decode_na(_z_zbuf_t *zbf, _z_ping_pong_result_t *r);
_z_ping_pong_result_t _z_ping_pong_decode(_z_zbuf_t *zbf);

int _z_frame_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_frame_t *msg);
void _z_frame_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_frame_result_t *r);
_z_frame_result_t _z_frame_decode(_z_zbuf_t *zbf, uint8_t header);

// /*------------------ Discovery Message ------------------*/
int _z_scout_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_scout_t *msg);
void _z_scout_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_scout_result_t *r);
_z_scout_result_t _z_scout_decode(_z_zbuf_t *zbf, uint8_t header);

int _z_hello_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_hello_t *msg);
void _z_hello_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_hello_result_t *r);
_z_hello_result_t _z_hello_decode(_z_zbuf_t *zbf, uint8_t header);

#endif /* ZENOH_PICO_TEST_H */
