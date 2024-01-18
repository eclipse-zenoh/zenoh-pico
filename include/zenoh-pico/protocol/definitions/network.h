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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_DEFINITIONS_NETWORK_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_DEFINITIONS_NETWORK_H

#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/definitions/message.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/keyexpr.h"
/* Network Messages */
#define _Z_MID_N_OAM 0x1f
#define _Z_MID_N_DECLARE 0x1e
#define _Z_MID_N_PUSH 0x1d
#define _Z_MID_N_REQUEST 0x1c
#define _Z_MID_N_RESPONSE 0x1b
#define _Z_MID_N_RESPONSE_FINAL 0x1a

/*=============================*/
/*        Network flags        */
/*=============================*/
#define _Z_FLAG_N_Z 0x80  // 1 << 7

// PUSH message flags:
//      N Named            if N==1 then the key expr has name/suffix
//      M Mapping          if M==1 then keyexpr mapping is the one declared by the sender, otherwise by the receiver
//      Z Extensions       if Z==1 then Zenoh extensions are present
#define _Z_FLAG_N_PUSH_N 0x20  // 1 << 5
#define _Z_FLAG_N_PUSH_M 0x40  // 1 << 6

// REQUEST message flags:
//      N Named            if N==1 then the key expr has name/suffix
//      M Mapping          if M==1 then keyexpr mapping is the one declared by the sender, otherwise by the receiver
//      Z Extensions       if Z==1 then Zenoh extensions are present
#define _Z_FLAG_N_REQUEST_N 0x20  // 1 << 5
#define _Z_FLAG_N_REQUEST_M 0x40  // 1 << 6

// RESPONSE message flags:
//      N Named            if N==1 then the key expr has name/suffix
//      M Mapping          if M==1 then keyexpr mapping is the one declared by the sender, otherwise by the receiver
//      Z Extensions       if Z==1 then Zenoh extensions are present
#define _Z_FLAG_N_RESPONSE_N 0x20  // 1 << 5
#define _Z_FLAG_N_RESPONSE_M 0x40  // 1 << 6

typedef struct {
    uint8_t _val;
} _z_n_qos_t;

#define _z_n_qos_make(express, nodrop, priority) \
    (_z_n_qos_t) { ._val = (((express) << 4) | ((nodrop) << 3) | (priority)) }
#define _Z_N_QOS_DEFAULT _z_n_qos_make(0, 0, 5)

// RESPONSE FINAL message flags:
//      Z Extensions       if Z==1 then Zenoh extensions are present
// #define _Z_FLAG_N_RESPONSE_X 0x20  // 1 << 5
// #define _Z_FLAG_N_RESPONSE_X 0x40  // 1 << 6

// Flags:
// - N: Named          if N==1 then the keyexpr has name/suffix
// - M: Mapping        if M==1 then keyexpr mapping is the one declared by the sender, otherwise by the receiver
// - Z: Extension      if Z==1 then at least one extension is present
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|M|N| REQUEST |
// +-+-+-+---------+
// ~ request_id:z32~
// +---------------+
// ~ key_scope:z16 ~
// +---------------+
// ~  key_suffix   ~  if N==1 -- <u8;z16>
// +---------------+
// ~   [req_exts]  ~  if Z==1
// +---------------+
// ~ ZenohMessage  ~
// +---------------+
//
typedef struct {
    _z_zint_t _rid;
    _z_keyexpr_t _key;
    _z_timestamp_t _ext_timestamp;
    _z_n_qos_t _ext_qos;
    z_query_target_t _ext_target;
    uint32_t _ext_budget;
    uint32_t _ext_timeout_ms;
    enum { _Z_REQUEST_QUERY, _Z_REQUEST_PUT, _Z_REQUEST_DEL, _Z_REQUEST_PULL } _tag;
    union {
        _z_msg_query_t _query;
        _z_msg_put_t _put;
        _z_msg_del_t _del;
        _z_msg_pull_t _pull;
    } _body;
} _z_n_msg_request_t;
typedef struct {
    _Bool ext_qos;
    _Bool ext_tstamp;
    _Bool ext_target;
    _Bool ext_budget;
    _Bool ext_timeout_ms;
    uint8_t n;
} _z_n_msg_request_exts_t;
_z_n_msg_request_exts_t _z_n_msg_request_needed_exts(const _z_n_msg_request_t *msg);
void _z_n_msg_request_clear(_z_n_msg_request_t *msg);

typedef struct {
    _Bool _is_put;
    union {
        _z_msg_del_t _del;
        _z_msg_put_t _put;
    } _body;
} _z_push_body_t;
_z_push_body_t _z_push_body_null(void);
_z_push_body_t _z_push_body_steal(_z_push_body_t *msg);
void _z_push_body_clear(_z_push_body_t *msg);

/*------------------ Response Final Message ------------------*/
// Flags:
// - Z: Extension      if Z==1 then at least one extension is present
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|M|N| ResFinal|
// +-+-+-+---------+
// ~ request_id:z32~
// +---------------+
// ~  [reply_exts] ~  if Z==1
// +---------------+
//
typedef struct {
    _z_zint_t _request_id;
} _z_n_msg_response_final_t;
void _z_n_msg_response_final_clear(_z_n_msg_response_final_t *msg);

// Flags:
// - N: Named          if N==1 then the keyexpr has name/suffix
// - M: Mapping        if M==1 then keyexpr mapping is the one declared by the sender, otherwise by the receiver
// - Z: Extension      if Z==1 then at least one extension is present
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|M|N|  PUSH   |
// +-+-+-+---------+
// ~ key_scope:z?  ~
// +---------------+
// ~  key_suffix   ~  if N==1 -- <u8;z16>
// +---------------+
// ~  [push_exts]  ~  if Z==1
// +---------------+
// ~ ZenohMessage  ~
// +---------------+
//
typedef struct {
    _z_keyexpr_t _key;
    _z_timestamp_t _timestamp;
    _z_n_qos_t _qos;
    _z_push_body_t _body;
} _z_n_msg_push_t;
void _z_n_msg_push_clear(_z_n_msg_push_t *msg);

/*------------------ Response Message ------------------*/
typedef struct {
    _z_timestamp_t _ext_timestamp;
    _z_zint_t _request_id;
    _z_keyexpr_t _key;
    _z_n_qos_t _ext_qos;
    struct {
        _z_id_t _zid;
        uint32_t _eid;
    } _ext_responder;
    enum {
        _Z_RESPONSE_BODY_REPLY,
        _Z_RESPONSE_BODY_ERR,
        _Z_RESPONSE_BODY_ACK,
        _Z_RESPONSE_BODY_PUT,
        _Z_RESPONSE_BODY_DEL,
    } _tag;
    union {
        _z_msg_reply_t _reply;
        _z_msg_err_t _err;
        _z_msg_ack_t _ack;
        _z_msg_put_t _put;
        _z_msg_del_t _del;
    } _body;
} _z_n_msg_response_t;
void _z_n_msg_response_clear(_z_n_msg_response_t *msg);

typedef struct {
    _z_declaration_t _decl;
    _z_timestamp_t _ext_timestamp;
    _z_n_qos_t _ext_qos;
} _z_n_msg_declare_t;
static inline void _z_n_msg_declare_clear(_z_n_msg_declare_t *msg) { _z_declaration_clear(&msg->_decl); }

/*------------------ Zenoh Message ------------------*/
typedef union {
    _z_n_msg_declare_t _declare;
    _z_n_msg_push_t _push;
    _z_n_msg_request_t _request;
    _z_n_msg_response_t _response;
    _z_n_msg_response_final_t _response_final;
} _z_network_body_t;
typedef struct {
    enum { _Z_N_DECLARE, _Z_N_PUSH, _Z_N_REQUEST, _Z_N_RESPONSE, _Z_N_RESPONSE_FINAL } _tag;
    _z_network_body_t _body;
} _z_network_message_t;
typedef _z_network_message_t _z_zenoh_message_t;
void _z_n_msg_clear(_z_network_message_t *m);
void _z_n_msg_free(_z_network_message_t **m);
inline static void _z_msg_clear(_z_zenoh_message_t *msg) { _z_n_msg_clear(msg); }
inline static void _z_msg_free(_z_zenoh_message_t **msg) { _z_n_msg_free(msg); }
_Z_ELEM_DEFINE(_z_network_message, _z_network_message_t, _z_noop_size, _z_n_msg_clear, _z_noop_copy)
_Z_VEC_DEFINE(_z_network_message, _z_network_message_t)

void _z_msg_fix_mapping(_z_zenoh_message_t *msg, uint16_t mapping);
_z_network_message_t _z_msg_make_pull(_z_keyexpr_t key, _z_zint_t pull_id);
_z_network_message_t _z_msg_make_query(_Z_MOVE(_z_keyexpr_t) key, _Z_MOVE(_z_bytes_t) parameters, _z_zint_t qid,
                                       z_consolidation_mode_t consolidation, _Z_MOVE(_z_value_t) value
#if Z_FEATURE_ATTACHMENT == 1
                                       ,
                                       z_attachment_t attachment
#endif
);
_z_network_message_t _z_n_msg_make_reply(_z_zint_t rid, _Z_MOVE(_z_keyexpr_t) key, _Z_MOVE(_z_value_t) value);
_z_network_message_t _z_n_msg_make_ack(_z_zint_t rid, _Z_MOVE(_z_keyexpr_t) key);
_z_network_message_t _z_n_msg_make_response_final(_z_zint_t rid);
_z_network_message_t _z_n_msg_make_declare(_z_declaration_t declaration);
_z_network_message_t _z_n_msg_make_push(_Z_MOVE(_z_keyexpr_t) key, _Z_MOVE(_z_push_body_t) body);

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_DEFINITIONS_NETWORK_H */
