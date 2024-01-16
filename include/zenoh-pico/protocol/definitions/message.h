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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_DEFINITIONS_MESSAGE_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_DEFINITIONS_MESSAGE_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/core.h"
/* Zenoh Messages */
#define _Z_MID_Z_OAM 0x00
#define _Z_MID_Z_PUT 0x01
#define _Z_MID_Z_DEL 0x02
#define _Z_MID_Z_QUERY 0x03
#define _Z_MID_Z_REPLY 0x04
#define _Z_MID_Z_ERR 0x05
#define _Z_MID_Z_ACK 0x06
#define _Z_MID_Z_PULL 0x07
#define _Z_MID_Z_LINK_STATE_LIST 0x10

/* Zenoh message flags */
#define _Z_FLAG_Z_Z 0x80
#define _Z_FLAG_Z_B 0x40  // 1 << 6 | QueryPayload      if B==1 then QueryPayload is present
#define _Z_FLAG_Z_D 0x20  // 1 << 5 | Dropping          if D==1 then the message can be dropped
#define _Z_FLAG_Z_F \
    0x20  // 1 << 5 | Final             if F==1 then this is the final message (e.g., ReplyContext, Pull)
#define _Z_FLAG_Z_I 0x40  // 1 << 6 | DataInfo          if I==1 then DataInfo is present
#define _Z_FLAG_Z_K 0x80  // 1 << 7 | ResourceKey       if K==1 then keyexpr is string
#define _Z_FLAG_Z_N 0x40  // 1 << 6 | MaxSamples        if N==1 then the MaxSamples is indicated
#define _Z_FLAG_Z_P 0x20  // 1 << 7 | Period            if P==1 then a period is present
#define _Z_FLAG_Z_Q 0x40  // 1 << 6 | QueryableKind     if Q==1 then the queryable kind is present
#define _Z_FLAG_Z_R \
    0x20  // 1 << 5 | Reliable          if R==1 then it concerns the reliable channel, best-effort otherwise
#define _Z_FLAG_Z_S 0x40  // 1 << 6 | SubMode           if S==1 then the declaration SubMode is indicated
#define _Z_FLAG_Z_T 0x20  // 1 << 5 | QueryTarget       if T==1 then the query target is present
#define _Z_FLAG_Z_X 0x00  // Unused flags are set to zero

#define _Z_FRAG_BUFF_BASE_SIZE 128  // Arbitrary base size of the buffer to encode a fragment message header

// Flags:
// - T: Timestamp      If T==1 then the timestamp if present
// - E: Encoding       If E==1 then the encoding is present
// - Z: Extension      If Z==1 then at least one extension is present
//
//   7 6 5 4 3 2 1 0
//  +-+-+-+-+-+-+-+-+
//  |Z|E|T|  REPLY  |
//  +-+-+-+---------+
//  ~ ts: <u8;z16>  ~  if T==1
//  +---------------+
//  ~   encoding    ~  if E==1
//  +---------------+
//  ~  [repl_exts]  ~  if Z==1
//  +---------------+
//  ~ pl: <u8;z32>  ~  -- Payload
//  +---------------+
typedef struct {
    _z_timestamp_t _timestamp;
    _z_value_t _value;
    _z_source_info_t _ext_source_info;
    z_consolidation_mode_t _ext_consolidation;
    _z_owned_encoded_attachment_t _ext_attachment;
} _z_msg_reply_t;
void _z_msg_reply_clear(_z_msg_reply_t *msg);
#define _Z_FLAG_Z_R_T 0x20
#define _Z_FLAG_Z_R_E 0x40

// Flags:
// - T: Timestamp      If T==1 then the timestamp if present
// - I: Infrastructure If I==1 then the error is related to the infrastructure else to the user
// - Z: Extension      If Z==1 then at least one extension is present
//
//   7 6 5 4 3 2 1 0
//  +-+-+-+-+-+-+-+-+
//  |Z|I|T|   ERR   |
//  +-+-+-+---------+
//  %   code:z16    %
//  +---------------+
//  ~ ts: <u8;z16>  ~  if T==1
//  +---------------+
//  ~  [err_exts]   ~  if Z==1
//  +---------------+
#define _Z_FLAG_Z_E_T 0x20
#define _Z_FLAG_Z_E_I 0x40
typedef struct {
    uint16_t _code;
    _Bool _is_infrastructure;
    _z_timestamp_t _timestamp;
    _z_source_info_t _ext_source_info;
    _z_value_t _ext_value;
} _z_msg_err_t;
void _z_msg_err_clear(_z_msg_err_t *err);

/// Flags:
/// - T: Timestamp      If T==1 then the timestamp if present
/// - X: Reserved
/// - Z: Extension      If Z==1 then at least one extension is present
///
///   7 6 5 4 3 2 1 0
///  +-+-+-+-+-+-+-+-+
///  |Z|X|T|   ACK   |
///  +-+-+-+---------+
///  ~ ts: <u8;z16>  ~  if T==1
///  +---------------+
///  ~  [err_exts]   ~  if Z==1
///  +---------------+
typedef struct {
    _z_timestamp_t _timestamp;
    _z_source_info_t _ext_source_info;
} _z_msg_ack_t;
#define _Z_FLAG_Z_A_T 0x20

/// Flags:
/// - T: Timestamp      If T==1 then the timestamp if present
/// - X: Reserved
/// - Z: Extension      If Z==1 then at least one extension is present
///
///   7 6 5 4 3 2 1 0
///  +-+-+-+-+-+-+-+-+
///  |Z|X|X|  PULL   |
///  +---------------+
///  ~  [pull_exts]  ~  if Z==1
///  +---------------+
typedef struct {
    _z_source_info_t _ext_source_info;
} _z_msg_pull_t;
static inline void _z_msg_pull_clear(_z_msg_pull_t *pull) { (void)pull; }

typedef struct {
    _z_timestamp_t _timestamp;
    _z_source_info_t _source_info;
} _z_m_push_commons_t;

typedef struct {
    _z_m_push_commons_t _commons;
} _z_msg_del_t;
static inline void _z_msg_del_clear(_z_msg_del_t *del) { (void)del; }
#define _Z_M_DEL_ID 0x02
#define _Z_FLAG_Z_D_T 0x20

typedef struct {
    _z_m_push_commons_t _commons;
    _z_bytes_t _payload;
    _z_encoding_t _encoding;
    _z_owned_encoded_attachment_t _attachment;
} _z_msg_put_t;
void _z_msg_put_clear(_z_msg_put_t *);
#define _Z_M_PUT_ID 0x01
#define _Z_FLAG_Z_P_E 0x40
#define _Z_FLAG_Z_P_T 0x20

/*------------------ Query Message ------------------*/
//   7 6 5 4 3 2 1 0
//  +-+-+-+-+-+-+-+-+
//  |Z|C|P|  QUERY  |
//  +-+-+-+---------+
//  ~ params        ~  if P==1 -- <utf8;z32>
//  +---------------+
//  ~ consolidation ~  if C==1 -- u8
//  +---------------+
//  ~ [qry_exts]    ~  if Z==1
//  +---------------+
#define _Z_FLAG_Z_Q_P 0x20  // 1 << 6 | Period            if P==1 then a period is present
typedef struct {
    _z_bytes_t _parameters;
    _z_source_info_t _ext_info;
    _z_value_t _ext_value;
    z_consolidation_mode_t _ext_consolidation;
    _z_owned_encoded_attachment_t _ext_attachment;
} _z_msg_query_t;
typedef struct {
    _Bool info;
    _Bool body;
    _Bool consolidation;
    _Bool attachment;
} _z_msg_query_reqexts_t;
_z_msg_query_reqexts_t _z_msg_query_required_extensions(const _z_msg_query_t *msg);
void _z_msg_query_clear(_z_msg_query_t *msg);

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_DEFINITIONS_MESSAGE_H */
