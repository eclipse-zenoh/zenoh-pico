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

#ifndef _ZENOH_NET_PICO_MSGCODEC_H
#define _ZENOH_NET_PICO_MSGCODEC_H

#define _ZENOH_PICO_FRAME_MESSAGES_VEC_SIZE 32

#include <stdint.h>
#include "zenoh-pico/net/private/codec.h"
#include "zenoh-pico/net/private/result.h"
#include "zenoh-pico/net/types.h"
#include "zenoh-pico/net/property.h"
#include "zenoh-pico/net/private/msg.h"

/*------------------ Result declarations ------------------*/
_ZN_RESULT_DECLARE(z_timestamp_t, timestamp)
_ZN_RESULT_DECLARE(zn_reskey_t, reskey)
_ZN_RESULT_DECLARE(zn_subinfo_t, subinfo)
_ZN_RESULT_DECLARE(zn_query_target_t, query_target)
_ZN_RESULT_DECLARE(zn_query_consolidation_t, query_consolidation)

_ZN_RESULT_DECLARE(_zn_payload_t, payload)
_ZN_RESULT_DECLARE(_zn_locators_t, locators)
_ZN_P_RESULT_DECLARE(_zn_attachment_t, attachment)
_ZN_P_RESULT_DECLARE(_zn_reply_context_t, reply_context)
_ZN_RESULT_DECLARE(_zn_scout_t, scout)
_ZN_RESULT_DECLARE(_zn_hello_t, hello)
_ZN_RESULT_DECLARE(_zn_init_t, init)
_ZN_RESULT_DECLARE(_zn_open_t, open)
_ZN_RESULT_DECLARE(_zn_close_t, close)
_ZN_RESULT_DECLARE(_zn_sync_t, sync)
_ZN_RESULT_DECLARE(_zn_ack_nack_t, ack_nack)
_ZN_RESULT_DECLARE(_zn_keep_alive_t, keep_alive)
_ZN_RESULT_DECLARE(_zn_ping_pong_t, ping_pong)
_ZN_RESULT_DECLARE(_zn_frame_t, frame)
_ZN_P_RESULT_DECLARE(_zn_session_message_t, session_message)

_ZN_RESULT_DECLARE(_zn_res_decl_t, res_decl)
_ZN_RESULT_DECLARE(_zn_pub_decl_t, pub_decl)
_ZN_RESULT_DECLARE(_zn_sub_decl_t, sub_decl)
_ZN_RESULT_DECLARE(_zn_qle_decl_t, qle_decl)
_ZN_RESULT_DECLARE(_zn_forget_res_decl_t, forget_res_decl)
_ZN_RESULT_DECLARE(_zn_forget_pub_decl_t, forget_pub_decl)
_ZN_RESULT_DECLARE(_zn_forget_sub_decl_t, forget_sub_decl)
_ZN_RESULT_DECLARE(_zn_forget_qle_decl_t, forget_qle_decl)
_ZN_RESULT_DECLARE(_zn_declaration_t, declaration)
_ZN_RESULT_DECLARE(_zn_declare_t, declare)
_ZN_RESULT_DECLARE(_zn_data_info_t, data_info)
_ZN_RESULT_DECLARE(_zn_data_t, data)
_ZN_RESULT_DECLARE(_zn_pull_t, pull)
_ZN_RESULT_DECLARE(_zn_query_t, query)
_ZN_P_RESULT_DECLARE(_zn_zenoh_message_t, zenoh_message)

/*------------------ Internal Zenoh-net Macros ------------------*/
#define _ZN_DECLARE_FREE(name) \
    void _zn_##name##_free(_zn_##name##_t *m, uint8_t header)

#define _ZN_DECLARE_FREE_NOH(name) \
    void _zn_##name##_free(_zn_##name##_t *m)

#define _ZN_DECLARE_ENCODE(name) \
    int _zn_##name##_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_##name##_t *m)

#define _ZN_DECLARE_ENCODE_NOH(name) \
    int _zn_##name##_encode(_z_wbuf_t *wbf, const _zn_##name##_t *m)

#define _ZN_DECLARE_DECODE(name)                                               \
    _zn_##name##_result_t _zn_##name##_decode(_z_rbuf_t *rbf, uint8_t header); \
    void _zn_##name##_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_##name##_result_t *r)

#define _ZN_DECLARE_DECODE_NOH(name)                           \
    _zn_##name##_result_t _zn_##name##_decode(_z_rbuf_t *rbf); \
    void _zn_##name##_decode_na(_z_rbuf_t *rbf, _zn_##name##_result_t *r)

#define _ZN_DECLARE_P_DECODE(name)                                               \
    _zn_##name##_p_result_t _zn_##name##_decode(_z_rbuf_t *rbf, uint8_t header); \
    void _zn_##name##_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_##name##_p_result_t *r)

#define _ZN_DECLARE_P_DECODE_NOH(name)                           \
    _zn_##name##_p_result_t _zn_##name##_decode(_z_rbuf_t *rbf); \
    void _zn_##name##_decode_na(_z_rbuf_t *rbf, _zn_##name##_p_result_t *r)

/*------------------ Session Message ------------------*/
_ZN_DECLARE_ENCODE(scout);
_ZN_DECLARE_DECODE(scout);

_ZN_DECLARE_ENCODE(hello);
_ZN_DECLARE_DECODE(hello);
_ZN_DECLARE_FREE(hello);

_ZN_DECLARE_ENCODE_NOH(session_message);
_ZN_DECLARE_P_DECODE_NOH(session_message);
_ZN_DECLARE_FREE_NOH(session_message);

/*------------------ Zenoh Message ------------------*/
_ZN_DECLARE_ENCODE_NOH(zenoh_message);
_ZN_DECLARE_P_DECODE_NOH(zenoh_message);
_ZN_DECLARE_FREE_NOH(zenoh_message);

/*------------------ Free Helpers ------------------*/
void _zn_reskey_free(zn_reskey_t *rk);

#endif /* ZENOH_NET_PICO_MSGCODEC_H */

// NOTE: the following headers are for unit testing only
#ifdef _ZENOH_NET_PICO_MSGCODEC_H_T
/*------------------ Message Fields ------------------*/
_ZN_DECLARE_ENCODE_NOH(payload);
_ZN_DECLARE_DECODE_NOH(payload);
_ZN_DECLARE_FREE_NOH(payload);

int _z_timestamp_encode(_z_wbuf_t *wbf, const z_timestamp_t *ts);
void _z_timestamp_decode_na(_z_rbuf_t *rbf, _zn_timestamp_result_t *r);
_zn_timestamp_result_t _z_timestamp_decode(_z_rbuf_t *rbf);
void _z_timestamp_free(z_timestamp_t *ts);

int _zn_subinfo_encode(_z_wbuf_t *wbf, const zn_subinfo_t *fld);
void _zn_subinfo_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_subinfo_result_t *r);
_zn_subinfo_result_t _zn_subinfo_decode(_z_rbuf_t *rbf, uint8_t header);
void _zn_subinfo_free(zn_subinfo_t *si);

int _zn_reskey_encode(_z_wbuf_t *wbf, uint8_t header, const zn_reskey_t *fld);
void _zn_reskey_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_reskey_result_t *r);
_zn_reskey_result_t _zn_reskey_decode(_z_rbuf_t *rbf, uint8_t header);

/*------------------ Message Decorators ------------------*/
_ZN_DECLARE_ENCODE_NOH(attachment);
_ZN_DECLARE_P_DECODE(attachment);
_ZN_DECLARE_FREE_NOH(attachment);

_ZN_DECLARE_ENCODE_NOH(reply_context);
_ZN_DECLARE_P_DECODE(reply_context);
_ZN_DECLARE_FREE_NOH(reply_context);

/*------------------ Zenoh Message ------------------*/
_ZN_DECLARE_ENCODE(res_decl);
_ZN_DECLARE_DECODE(res_decl);
_ZN_DECLARE_FREE_NOH(res_decl);

_ZN_DECLARE_ENCODE(pub_decl);
_ZN_DECLARE_DECODE(pub_decl);
_ZN_DECLARE_FREE_NOH(pub_decl);

_ZN_DECLARE_ENCODE(sub_decl);
_ZN_DECLARE_DECODE(sub_decl);
_ZN_DECLARE_FREE_NOH(sub_decl);

_ZN_DECLARE_ENCODE(qle_decl);
_ZN_DECLARE_DECODE(qle_decl);
_ZN_DECLARE_FREE_NOH(qle_decl);

_ZN_DECLARE_ENCODE_NOH(forget_res_decl);
_ZN_DECLARE_DECODE_NOH(forget_res_decl);
_ZN_DECLARE_FREE_NOH(forget_res_decl);

_ZN_DECLARE_ENCODE(forget_pub_decl);
_ZN_DECLARE_DECODE(forget_pub_decl);
_ZN_DECLARE_FREE_NOH(forget_pub_decl);

_ZN_DECLARE_ENCODE(forget_sub_decl);
_ZN_DECLARE_DECODE(forget_sub_decl);
_ZN_DECLARE_FREE_NOH(forget_sub_decl);

_ZN_DECLARE_ENCODE(forget_qle_decl);
_ZN_DECLARE_DECODE(forget_qle_decl);
_ZN_DECLARE_FREE_NOH(forget_qle_decl);

_ZN_DECLARE_ENCODE_NOH(declaration);
_ZN_DECLARE_DECODE_NOH(declaration);
_ZN_DECLARE_FREE_NOH(declaration);

_ZN_DECLARE_ENCODE_NOH(declare);
_ZN_DECLARE_DECODE_NOH(declare);
_ZN_DECLARE_FREE_NOH(declare);

_ZN_DECLARE_ENCODE_NOH(data_info);
_ZN_DECLARE_DECODE_NOH(data_info);
_ZN_DECLARE_FREE_NOH(data_info);

_ZN_DECLARE_ENCODE(data);
_ZN_DECLARE_DECODE(data);
_ZN_DECLARE_FREE_NOH(data);

_ZN_DECLARE_ENCODE(pull);
_ZN_DECLARE_DECODE(pull);
_ZN_DECLARE_FREE_NOH(pull);

_ZN_DECLARE_ENCODE(query);
_ZN_DECLARE_DECODE(query);
_ZN_DECLARE_FREE_NOH(query);

/*------------------ Session Message ------------------*/
_ZN_DECLARE_ENCODE(init);
_ZN_DECLARE_DECODE(init);
_ZN_DECLARE_FREE(init);

_ZN_DECLARE_ENCODE(open);
_ZN_DECLARE_DECODE(open);
_ZN_DECLARE_FREE(open);

_ZN_DECLARE_ENCODE(close);
_ZN_DECLARE_DECODE(close);
_ZN_DECLARE_FREE(close);

_ZN_DECLARE_ENCODE(sync);
_ZN_DECLARE_DECODE(sync);
_ZN_DECLARE_FREE_NOH(sync);

_ZN_DECLARE_ENCODE(ack_nack);
_ZN_DECLARE_DECODE(ack_nack);
_ZN_DECLARE_FREE_NOH(ack_nack);

_ZN_DECLARE_ENCODE(keep_alive);
_ZN_DECLARE_DECODE(keep_alive);
_ZN_DECLARE_FREE(keep_alive);

_ZN_DECLARE_ENCODE_NOH(ping_pong);
_ZN_DECLARE_DECODE_NOH(ping_pong);
_ZN_DECLARE_FREE_NOH(ping_pong);

_ZN_DECLARE_ENCODE(frame);
_ZN_DECLARE_DECODE(frame);
_ZN_DECLARE_FREE(frame);

#endif /* _ZENOH_NET_PICO_MSGCODEC_H_T */
