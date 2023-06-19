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

#include <stdint.h>
#define _ZENOH_PICO_FRAME_MESSAGES_VEC_SIZE 32

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Zenoh Message ------------------*/
int8_t _z_scouting_message_encode(_z_wbuf_t *buf, const _z_scouting_message_t *msg);
int8_t _z_scouting_message_decode(_z_scouting_message_t *msg, _z_zbuf_t *buf);
int8_t _z_scouting_message_decode_na(_z_scouting_message_t *msg, _z_zbuf_t *buf);

int8_t _z_transport_message_encode(_z_wbuf_t *buf, const _z_transport_message_t *msg);
int8_t _z_transport_message_decode(_z_transport_message_t *msg, _z_zbuf_t *buf);

int8_t _z_network_message_encode(_z_wbuf_t *buf, const _z_network_message_t *msg);
int8_t _z_network_message_decode(_z_network_message_t *msg, _z_zbuf_t *buf);
int8_t _z_network_message_decode_na(_z_network_message_t *msg, _z_zbuf_t *buf);

int8_t _z_zenoh_message_encode(_z_wbuf_t *buf, const _z_zenoh_message_t *msg);
int8_t _z_zenoh_message_decode(_z_zenoh_message_t *msg, _z_zbuf_t *buf);
int8_t _z_zenoh_message_decode_na(_z_zenoh_message_t *msg, _z_zbuf_t *buf);

int8_t _z_keyexpr_encode(_z_wbuf_t *buf, _Bool has_suffix, const _z_keyexpr_t *ke);
int8_t _z_keyexpr_decode(_z_keyexpr_t *ke, _z_zbuf_t *buf, _Bool has_suffix);

int8_t _z_timestamp_encode(_z_wbuf_t *buf, const _z_timestamp_t *ts);
int8_t _z_timestamp_decode(_z_timestamp_t *ts, _z_zbuf_t *buf);

int8_t _z_push_body_encode(_z_wbuf_t *buf, const _z_push_body_t *ts);
int8_t _z_push_body_decode(_z_push_body_t *ts, _z_zbuf_t *buf);
#endif /* ZENOH_PICO_MSGCODEC_H */

// NOTE: the following headers are for unit testing only
#ifdef ZENOH_PICO_TEST_H
// ------------------ Message Fields ------------------
int8_t _z_payload_encode(_z_wbuf_t *buf, const _z_payload_t *pld);
int8_t _z_payload_decode(_z_payload_t *pld, _z_zbuf_t *buf);
int8_t _z_payload_decode_na(_z_payload_t *pld, _z_zbuf_t *buf);

int8_t _z_subinfo_encode(_z_wbuf_t *buf, const _z_subinfo_t *si);
int8_t _z_subinfo_decode(_z_subinfo_t *si, _z_zbuf_t *buf, uint8_t header);
int8_t _z_subinfo_decode_na(_z_subinfo_t *si, _z_zbuf_t *buf, uint8_t header);

int8_t _z_data_info_encode(_z_wbuf_t *buf, const _z_data_info_t *di);
int8_t _z_data_info_decode(_z_data_info_t *di, _z_zbuf_t *buf);
int8_t _z_data_info_decode_na(_z_data_info_t *di, _z_zbuf_t *buf);

// ------------------ Zenoh Message ------------------
int8_t _z_res_decl_encode(_z_wbuf_t *buf, uint8_t header, const _z_res_decl_t *dcl);
int8_t _z_res_decl_decode(_z_res_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);
int8_t _z_res_decl_decode_na(_z_res_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);

int8_t _z_pub_decl_encode(_z_wbuf_t *buf, uint8_t header, const _z_pub_decl_t *dcl);
int8_t _z_pub_decl_decode(_z_pub_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);
int8_t _z_pub_decl_decode_na(_z_pub_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);

int8_t _z_sub_decl_encode(_z_wbuf_t *buf, uint8_t header, const _z_sub_decl_t *dcl);
int8_t _z_sub_decl_decode(_z_sub_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);
int8_t _z_sub_decl_decode_na(_z_sub_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);

int8_t _z_qle_decl_encode(_z_wbuf_t *buf, uint8_t header, const _z_qle_decl_t *dcl);
int8_t _z_qle_decl_decode(_z_qle_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);
int8_t _z_qle_decl_decode_na(_z_qle_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);

int8_t _z_forget_res_decl_encode(_z_wbuf_t *buf, const _z_forget_res_decl_t *dcl);
int8_t _z_forget_res_decl_decode(_z_forget_res_decl_t *dcl, _z_zbuf_t *buf);
int8_t _z_forget_res_decl_decode_na(_z_forget_res_decl_t *dcl, _z_zbuf_t *buf);

int8_t _z_forget_pub_decl_encode(_z_wbuf_t *buf, uint8_t header, const _z_forget_pub_decl_t *dcl);
int8_t _z_forget_pub_decl_decode(_z_forget_pub_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);
int8_t _z_forget_pub_decl_decode_na(_z_forget_pub_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);

int8_t _z_forget_sub_decl_encode(_z_wbuf_t *buf, uint8_t header, const _z_forget_sub_decl_t *dcl);
int8_t _z_forget_sub_decl_decode(_z_forget_sub_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);
int8_t _z_forget_sub_decl_decode_na(_z_forget_sub_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);

int8_t _z_forget_qle_decl_encode(_z_wbuf_t *buf, uint8_t header, const _z_forget_qle_decl_t *dcl);
int8_t _z_forget_qle_decl_decode(_z_forget_qle_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);
int8_t _z_forget_qle_decl_decode_na(_z_forget_qle_decl_t *dcl, _z_zbuf_t *buf, uint8_t header);

int8_t _z_reply_context_encode(_z_wbuf_t *buf, const _z_reply_context_t *rc);
int8_t _z_reply_context_decode(_z_reply_context_t **rc, _z_zbuf_t *buf, uint8_t header);
int8_t _z_reply_context_decode_na(_z_reply_context_t *rc, _z_zbuf_t *buf, uint8_t header);

int8_t _z_data_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_data_t *msg);
int8_t _z_data_decode_na(_z_msg_data_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_data_decode(_z_msg_data_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_pull_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_pull_t *msg);
int8_t _z_pull_decode_na(_z_msg_pull_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_pull_decode(_z_msg_pull_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_query_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_query_t *msg);
int8_t _z_query_decode_na(_z_msg_query_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_query_decode(_z_msg_query_t *msg, _z_zbuf_t *zbf, uint8_t header);

// ------------------ Network Message ------------------
int8_t _z_declare_encode(_z_wbuf_t *wbf, const _z_n_msg_declare_t *msg);
int8_t _z_declare_decode(_z_n_msg_declare_t *msg, _z_zbuf_t *zbf);
int8_t _z_declare_decode_na(_z_n_msg_declare_t *msg, _z_zbuf_t *zbf);

int8_t _z_push_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_push_t *msg);
int8_t _z_push_decode(_z_n_msg_push_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_request_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_request_t *msg);
int8_t _z_request_decode(_z_n_msg_request_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_request_decode_na(_z_n_msg_request_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_response_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_response_t *msg);
int8_t _z_response_decode(_z_n_msg_response_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_response_decode_na(_z_n_msg_response_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_response_final_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_response_final_t *msg);
int8_t _z_response_final_decode(_z_n_msg_response_final_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_response_final_decode_na(_z_n_msg_response_final_t *msg, _z_zbuf_t *zbf, uint8_t header);

// ------------------ Transport Message ------------------
int8_t _z_join_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_join_t *msg);
int8_t _z_join_decode(_z_t_msg_join_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_init_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_init_t *msg);
int8_t _z_init_decode(_z_t_msg_init_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_open_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_open_t *msg);
int8_t _z_open_decode(_z_t_msg_open_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_close_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_close_t *msg);
int8_t _z_close_decode(_z_t_msg_close_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_keep_alive_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_keep_alive_t *msg);
int8_t _z_keep_alive_decode(_z_t_msg_keep_alive_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_frame_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_frame_t *msg);
int8_t _z_frame_decode(_z_t_msg_frame_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_fragment_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_fragment_t *msg);
int8_t _z_fragment_decode(_z_t_msg_fragment_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_extensions_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_ext_vec_t *v_ext);
int8_t _z_extensions_decode(_z_msg_ext_vec_t *v_ext, _z_zbuf_t *zbf, uint8_t header);

// ------------------ Discovery Message ------------------
int8_t _z_scout_encode(_z_wbuf_t *wbf, uint8_t header, const _z_s_msg_scout_t *msg);
int8_t _z_scout_decode_na(_z_s_msg_scout_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_scout_decode(_z_s_msg_scout_t *msg, _z_zbuf_t *zbf, uint8_t header);

int8_t _z_hello_encode(_z_wbuf_t *wbf, uint8_t header, const _z_s_msg_hello_t *msg);
int8_t _z_hello_decode_na(_z_s_msg_hello_t *msg, _z_zbuf_t *zbf, uint8_t header);
int8_t _z_hello_decode(_z_s_msg_hello_t *msg, _z_zbuf_t *zbf, uint8_t header);

#endif /* ZENOH_PICO_TEST_H */
