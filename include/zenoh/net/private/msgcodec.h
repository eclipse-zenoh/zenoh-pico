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
void _zn_zenoh_message_encode(z_iobuf_t *buf, const _zn_zenoh_message_t *m);
_zn_zenoh_message_p_result_t z_zenoh_message_decode(z_iobuf_t *buf);
void _zn_zenoh_message_decode_na(z_iobuf_t *buf, _zn_zenoh_message_p_result_t *r);

#endif /* ZENOH_C_NET_MSGCODEC_H */

// NOTE: the following headers are for unit tests only
#ifdef ZENOH_C_NET_MSGCODEC_T
/*------------------ Message Fields ------------------*/
void _zn_payload_encode(z_iobuf_t *buf, const z_iobuf_t *bs);
z_iobuf_t _zn_payload_decode(z_iobuf_t *buf);

void _zn_timestamp_encode(z_iobuf_t *buf, const z_timestamp_t *ts);
_zn_timestamp_result_t _zn_timestamp_decode(z_iobuf_t *buf);

void _zn_sub_mode_encode(z_iobuf_t *buf, const zn_sub_mode_t *fld);
_zn_sub_mode_result_t _zn_sub_mode_decode(z_iobuf_t *buf);

void _zn_res_key_encode(z_iobuf_t *buf, const zn_res_key_t *fld);
_zn_res_key_result_t _zn_res_key_decode(uint8_t flags, z_iobuf_t *buf);

/*------------------ Message Decorators ------------------*/
void _zn_attachment_encode(z_iobuf_t *buf, const _zn_attachment_t *msg);
_zn_attachment_result_t _zn_attachment_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_reply_context_encode(z_iobuf_t *buf, const _zn_reply_context_t *msg);
_zn_reply_context_result_t _zn_reply_context_decode(uint8_t flags, z_iobuf_t *buf);

/*------------------ Zenoh Message ------------------*/
void _zn_res_decl_encode(z_iobuf_t *buf, const _zn_res_decl_t *dcl);
_zn_res_decl_result_t _zn_res_decl_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_pub_decl_encode(z_iobuf_t *buf, const _zn_pub_decl_t *dcl);
_zn_pub_decl_result_t _zn_pub_decl_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_sub_decl_encode(z_iobuf_t *buf, const _zn_sub_decl_t *dcl);
_zn_sub_decl_result_t _zn_sub_decl_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_qle_decl_encode(z_iobuf_t *buf, const _zn_qle_decl_t *dcl);
_zn_qle_decl_result_t _zn_qle_decl_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_forget_res_decl_encode(z_iobuf_t *buf, const _zn_forget_res_decl_t *dcl);
_zn_forget_res_decl_result_t _zn_forget_res_decl_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_forget_pub_decl_encode(z_iobuf_t *buf, const _zn_forget_pub_decl_t *dcl);
_zn_forget_pub_decl_result_t _zn_forget_pub_decl_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_forget_sub_decl_encode(z_iobuf_t *buf, const _zn_forget_sub_decl_t *dcl);
_zn_forget_sub_decl_result_t _zn_forget_sub_decl_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_forget_qle_decl_encode(z_iobuf_t *buf, const _zn_forget_qle_decl_t *dcl);
_zn_forget_qle_decl_result_t _zn_forget_qle_decl_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_declaration_encode(z_iobuf_t *buf, _zn_declaration_t *dcl);
_zn_declaration_result_t _zn_declaration_decode(z_iobuf_t *buf);

void _zn_declare_encode(z_iobuf_t *buf, const _zn_declare_t *msg);
_zn_declare_result_t _zn_declare_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_data_info_encode(z_iobuf_t *buf, const zn_data_info_t *fld);
_zn_data_info_result_t _zn_data_info_decode(z_iobuf_t *buf);

void _zn_data_encode(z_iobuf_t *buf, const _zn_data_t *msg);
_zn_data_result_t _zn_data_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_unit_encode(z_iobuf_t *buf, const _zn_unit_t *msg);
_zn_unit_result_t _zn_unit_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_pull_encode(z_iobuf_t *buf, const _zn_pull_t *msg);
_zn_pull_result_t _zn_pull_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_query_encode(z_iobuf_t *buf, const _zn_query_t *msg);
_zn_query_result_t _zn_query_decode(uint8_t flags, z_iobuf_t *buf);

/*------------------ Session Message ------------------*/
void _zn_scout_encode(z_iobuf_t *buf, const _zn_scout_t *msg);
_zn_scout_result_t z_scout_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_hello_encode(z_iobuf_t *buf, const _zn_hello_t *msg);
_zn_hello_result_t z_hello_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_open_encode(z_iobuf_t *buf, const _zn_open_t *msg);
_zn_open_result_t z_open_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_accept_encode(z_iobuf_t *buf, const _zn_accept_t *msg);
_zn_accept_result_t _zn_accept_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_close_encode(z_iobuf_t *buf, const _zn_close_t *msg);
_zn_close_result_t _zn_close_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_sync_encode(z_iobuf_t *buf, const _zn_sync_t *msg);
_zn_sync_result_t _zn_sync_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_ack_nack_encode(z_iobuf_t *buf, const _zn_ack_nack_t *msg);
_zn_ack_nack_result_t _zn_ack_nack_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_keep_alive_encode(z_iobuf_t *buf, const _zn_keep_alive_t *msg);
_zn_keep_alive_result_t _zn_keep_alive_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_ping_pong_encode(z_iobuf_t *buf, const _zn_ping_pong_t *msg);
_zn_ping_pong_result_t _zn_ping_pong_decode(uint8_t flags, z_iobuf_t *buf);

void _zn_frame_encode(z_iobuf_t *buf, const _zn_frame_t *msg);
_zn_frame_result_t _zn_frame_decode(uint8_t flags, z_iobuf_t *buf);

#endif /* ZENOH_C_NET_MSGCODEC_T */
