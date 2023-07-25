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

#ifndef ZENOH_PICO_PROTOCOL_MSG_H
#define ZENOH_PICO_PROTOCOL_MSG_H

#include <stdbool.h>

#include "zenoh-pico/collections/array.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/protocol/core.h"

#define _Z_DECLARE_CLEAR(layer, name) void _z_##layer##_msg_clear_##name(_z_##name##_t *m, uint8_t header)

#define _Z_DECLARE_CLEAR_NOH(layer, name) void _z_##layer##_msg_clear_##name(_z_##name##_t *m)

// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
#define _Z_MSG_LEN_ENC_SIZE 2

/*=============================*/
/*         Message IDs         */
/*=============================*/
/* Transport Messages */
#define _Z_MID_JOIN 0x00
#define _Z_MID_SCOUT 0x01
#define _Z_MID_HELLO 0x02
#define _Z_MID_INIT 0x03
#define _Z_MID_OPEN 0x04
#define _Z_MID_CLOSE 0x05
#define _Z_MID_SYNC 0x06
#define _Z_MID_ACK_NACK 0x07
#define _Z_MID_KEEP_ALIVE 0x08
#define _Z_MID_PING_PONG 0x09
#define _Z_MID_FRAME 0x0a
/* Zenoh Messages */
#define _Z_MID_DECLARE 0x0b
#define _Z_MID_DATA 0x0c
#define _Z_MID_QUERY 0x0d
#define _Z_MID_PULL 0x0e
#define _Z_MID_UNIT 0x0f
#define _Z_MID_LINK_STATE_LIST 0x10
/* Message decorators */
#define _Z_MID_PRIORITY 0x1c
#define _Z_MID_ROUTING_CONTEXT 0x1d
#define _Z_MID_REPLY_CONTEXT 0x1e
#define _Z_MID_ATTACHMENT 0x1f

/*=============================*/
/*        Message flags        */
/*=============================*/
/* Transport message flags */
#define _Z_FLAG_T_A 0x20  // 1 << 5 | Ack              if A==1 then the message is an acknowledgment
#define _Z_FLAG_T_C 0x40  // 1 << 6 | Count            if C==1 then number of unacknowledged messages is present
#define _Z_FLAG_T_E 0x80  // 1 << 7 | End              if E==1 then it is the last FRAME fragment
#define _Z_FLAG_T_F 0x40  // 1 << 6 | Fragment         if F==1 then the FRAME is a fragment
#define _Z_FLAG_T_I 0x20  // 1 << 5 | PeerID           if I==1 then the PeerID is present or requested
#define _Z_FLAG_T_K 0x40  // 1 << 6 | CloseLink        if K==1 then close the transport link only
#define _Z_FLAG_T_L 0x80  // 1 << 7 | Locators         if L==1 then Locators are present
#define _Z_FLAG_T_M 0x20  // 1 << 5 | Mask             if M==1 then a Mask is present
#define _Z_FLAG_T_O 0x80  // 1 << 7 | Options          if O==1 then Options are present
#define _Z_FLAG_T_P 0x20  // 1 << 5 | PingOrPong       if P==1 then the message is Ping, otherwise is Pong
#define _Z_FLAG_T_R \
    0x20  // 1 << 5 | Reliable         if R==1 then it concerns the reliable channel, best-effort otherwise
#define _Z_FLAG_T_S 0x40   // 1 << 6 | SN Resolution    if S==1 then the SN Resolution is present
#define _Z_FLAG_T_T1 0x20  // 1 << 5 | TimeRes          if T==1 then the time resolution is in seconds
#define _Z_FLAG_T_T2 0x40  // 1 << 6 | TimeRes          if T==1 then the time resolution is in seconds
#define _Z_FLAG_T_W 0x40   // 1 << 6 | WhatAmI          if W==1 then WhatAmI is indicated
#define _Z_FLAG_T_Z \
    0x20  // 1 << 5 | MixedSlices      if Z==1 then the payload contains a mix of raw and shm_info payload
#define _Z_FLAG_T_X 0x00  // Unused flags are set to zero

/* Zenoh message flags */
#define _Z_FLAG_Z_B 0x40  // 1 << 6 | QueryPayload      if B==1 then QueryPayload is present
#define _Z_FLAG_Z_D 0x20  // 1 << 5 | Dropping          if D==1 then the message can be dropped
#define _Z_FLAG_Z_F \
    0x20  // 1 << 5 | Final             if F==1 then this is the final message (e.g., ReplyContext, Pull)
#define _Z_FLAG_Z_I 0x40  // 1 << 6 | DataInfo          if I==1 then DataInfo is present
#define _Z_FLAG_Z_K 0x80  // 1 << 7 | ResourceKey       if K==1 then keyexpr is string
#define _Z_FLAG_Z_N 0x40  // 1 << 6 | MaxSamples        if N==1 then the MaxSamples is indicated
#define _Z_FLAG_Z_P 0x80  // 1 << 7 | Period            if P==1 then a period is present
#define _Z_FLAG_Z_Q 0x40  // 1 << 6 | QueryableKind     if Q==1 then the queryable kind is present
#define _Z_FLAG_Z_R \
    0x20  // 1 << 5 | Reliable          if R==1 then it concerns the reliable channel, best-effort otherwise
#define _Z_FLAG_Z_S 0x40  // 1 << 6 | SubMode           if S==1 then the declaration SubMode is indicated
#define _Z_FLAG_Z_T 0x20  // 1 << 5 | QueryTarget       if T==1 then the query target is present
#define _Z_FLAG_Z_X 0x00  // Unused flags are set to zero

/* Init option flags */
#define _Z_OPT_INIT_QOS 0x01  // 1 << 0 | QoS       if QOS==1 then the session supports QoS
#define _Z_OPT_JOIN_QOS 0x01  // 1 << 0 | QoS       if QOS==1 then the session supports QoS

/*=============================*/
/*       Message header        */
/*=============================*/
#define _Z_MID_MASK 0x1f
#define _Z_FLAGS_MASK 0xe0

/*=============================*/
/*       Message helpers       */
/*=============================*/
#define _Z_MID(h) (_Z_MID_MASK & h)
#define _Z_FLAGS(h) (_Z_FLAGS_MASK & h)
#define _Z_HAS_FLAG(h, f) ((h & f) != 0)
#define _Z_SET_FLAG(h, f) (h |= f)

/*=============================*/
/*       Declaration IDs       */
/*=============================*/
#define _Z_DECL_RESOURCE 0x01
#define _Z_DECL_PUBLISHER 0x02
#define _Z_DECL_SUBSCRIBER 0x03
#define _Z_DECL_QUERYABLE 0x04
#define _Z_DECL_FORGET_RESOURCE 0x11
#define _Z_DECL_FORGET_PUBLISHER 0x12
#define _Z_DECL_FORGET_SUBSCRIBER 0x13
#define _Z_DECL_FORGET_QUERYABLE 0x14

/*=============================*/
/*        Close reasons        */
/*=============================*/
#define _Z_CLOSE_GENERIC 0x00
#define _Z_CLOSE_UNSUPPORTED 0x01
#define _Z_CLOSE_INVALID 0x02
#define _Z_CLOSE_MAX_TRANSPORTS 0x03
#define _Z_CLOSE_MAX_LINKS 0x04
#define _Z_CLOSE_EXPIRED 0x05

/*=============================*/
/*       DataInfo flags        */
/*=============================*/
#define _Z_DATA_INFO_SLICED 0x01  // 1 << 0
#define _Z_DATA_INFO_KIND 0x02    // 1 << 1
#define _Z_DATA_INFO_ENC 0x04     // 1 << 2
#define _Z_DATA_INFO_TSTAMP 0x08  // 1 << 3
// Reserved: bits 4-6
#define _Z_DATA_INFO_SRC_ID 0x80   // 1 << 7
#define _Z_DATA_INFO_SRC_SN 0x100  // 1 << 8

/*------------------ Payload field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// ~    Length     ~
// +---------------+
// ~    Buffer     ~
// +---------------+
//
typedef _z_bytes_t _z_payload_t;
void _z_payload_clear(_z_payload_t *p);

/*------------------ Locators Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// ~  Num of Locs  ~
// +---------------+
// ~   [Locator]   ~
// +---------------+
//
// NOTE: Locators is an array of strings and are encoded as such

/*=============================*/
/*     Message decorators      */
/*=============================*/
// # Attachment decorator
//
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// The Attachment can decorate any message (i.e., TransportMessage and ZenohMessage) and it allows to
// append to the message any additional information. Since the information contained in the
// Attachement is relevant only to the layer that provided them (e.g., Transport, Zenoh, User) it
// is the duty of that layer to serialize and de-serialize the attachment whenever deemed necessary.
// The attachement always contains serialized properties.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|Z|  ATTCH  |
// +-+-+-+---------+
// ~   Attachment  ~
// +---------------+
//
typedef struct {
    _z_payload_t _payload;
    uint8_t _header;
} _z_attachment_t;
void _z_t_msg_clear_attachment(_z_attachment_t *a);

/*------------------ ReplyContext Decorator ------------------*/
// The ReplyContext is a message decorator for either:
//   - the Data messages that results from a query
//   - or a Unit message in case the message is a REPLY_FINAL.
//  The replier-id (eval or storage id) is represented as a byte-array.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|F|  R_CTX  |
// +-+-+-+---------+
// ~      qid      ~
// +---------------+
// ~   replier_id  ~ if F==0
// +---------------+
//
// - if F==1 then the message is a REPLY_FINAL
//
typedef struct {
    _z_bytes_t _replier_id;
    _z_zint_t _qid;
    uint8_t _header;
} _z_reply_context_t;
void _z_msg_clear_reply_context(_z_reply_context_t *rc);

// -- Priority decorator
//
// The Priority is a message decorator containing
// informations related to the priority of the frame/zenoh message.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// | ID  |  Prio   |
// +-+-+-+---------+
//
// WARNING: zenoh-pico does not support priorities and QoS.

/*=============================*/
/*       Zenoh Messages        */
/*=============================*/
/*------------------ ResKey Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// ~      RID      ~
// +---------------+
// ~    Suffix     ~ if K==1
// +---------------+
//
void _z_keyexpr_clear(_z_keyexpr_t *rk);
void _z_keyexpr_free(_z_keyexpr_t **rk);

/*------------------ Resource Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|   RES   |
// +---------------+
// ~      RID      ~
// +---------------+
// ~    ResKey     ~ if K==1 then keyexpr is string
// +---------------+
//
typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _id;
} _z_res_decl_t;
void _z_msg_clear_declaration_resource(_z_res_decl_t *dcl);

/*------------------ Forget Resource Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|X|  F_RES  |
// +---------------+
// ~      RID      ~
// +---------------+
//
typedef struct {
    _z_zint_t _rid;
} _z_forget_res_decl_t;
void _z_msg_clear_declaration_forget_resource(_z_forget_res_decl_t *dcl);

/*------------------ Publisher Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|   PUB   |
// +---------------+
// ~    ResKey     ~ if K==1 then keyexpr is string
// +---------------+
//
typedef struct {
    _z_keyexpr_t _key;
} _z_pub_decl_t;
void _z_msg_clear_declaration_publisher(_z_pub_decl_t *dcl);

/*------------------ Forget Publisher Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|  F_PUB  |
// +---------------+
// ~    ResKey     ~ if K==1 then keyexpr is string
// +---------------+
//
typedef struct {
    _z_keyexpr_t _key;
} _z_forget_pub_decl_t;
void _z_msg_clear_declaration_forget_publisher(_z_forget_pub_decl_t *dcl);

/*------------------ SubInfo Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// |P|X|X|  SM_ID  |
// +---------------+
// ~    Period     ~ if P==1
// +---------------+
//
void _z_subinfo_clear(_z_subinfo_t *si);

/*------------------ Subscriber Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|S|R|   SUB   |
// +---------------+
// ~    ResKey     ~ if K==1 then keyexpr is string
// +---------------+
// ~    SubInfo    ~ if S==1. Otherwise: SubMode=Push
// +---------------+
//
// - if R==1 then the subscription is reliable, best-effort otherwise.
//
typedef struct {
    _z_keyexpr_t _key;
    _z_subinfo_t _subinfo;
} _z_sub_decl_t;
void _z_msg_clear_declaration_subscriber(_z_sub_decl_t *dcl);

/*------------------ Forget Subscriber Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|  F_SUB  |
// +---------------+
// ~    ResKey     ~ if K==1 then keyexpr is string
// +---------------+
//
typedef struct {
    _z_keyexpr_t _key;
} _z_forget_sub_decl_t;
void _z_msg_clear_declaration_forget_subscriber(_z_forget_sub_decl_t *dcl);

/*------------------ Queryable Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|Q|X|  QABLE  |
// +---------------+
// ~     ResKey    ~ if K==1 then keyexpr is string
// +---------------+
// ~   QablInfo    ~ if Q==1
// +---------------+
//
typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _complete;
    _z_zint_t _distance;
} _z_qle_decl_t;
void _z_msg_clear_declaration_queryable(_z_qle_decl_t *dcl);

/*------------------ Forget Queryable Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X| F_QABLE |
// +---------------+
// ~    ResKey     ~ if K==1 then keyexpr is string
// +---------------+
//
typedef struct {
    _z_keyexpr_t _key;
} _z_forget_qle_decl_t;
void _z_msg_clear_declaration_forget_queryable(_z_forget_qle_decl_t *dcl);

/*------------------ Declaration  Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|X| DECLARE |
// +-+-+-+---------+
// ~  Num of Decl  ~
// +---------------+
// ~ [Declaration] ~
// +---------------+
//
typedef struct {
    union {
        _z_res_decl_t _res;
        _z_forget_res_decl_t _forget_res;
        _z_pub_decl_t _pub;
        _z_forget_pub_decl_t _forget_pub;
        _z_sub_decl_t _sub;
        _z_forget_sub_decl_t _forget_sub;
        _z_qle_decl_t _qle;
        _z_forget_qle_decl_t _forget_qle;
    } _body;
    uint8_t _header;
} _z_declaration_t;

void _z_msg_clear_declaration(_z_declaration_t *dcl);
_Z_ELEM_DEFINE(_z_declaration, _z_declaration_t, _z_noop_size, _z_msg_clear_declaration, _z_noop_copy)
_Z_ARRAY_DEFINE(_z_declaration, _z_declaration_t)

typedef struct {
    _z_declaration_array_t _declarations;
} _z_msg_declare_t;
void _z_msg_clear_declare(_z_msg_declare_t *dcl);

/*------------------ Timestamp Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// ~     Time      ~  Encoded as _z_zint_t
// +---------------+
// ~      ID       ~
// +---------------+
//
void _z_timestamp_clear(_z_timestamp_t *ts);

/*------------------ Data Info Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// ~    options    ~
// +---------------+
// ~      kind     ~ if options & (1 << 1)
// +---------------+
// ~   encoding    ~ if options & (1 << 2)
// +---------------+
// ~   timestamp   ~ if options & (1 << 3)
// +---------------+
// ~   source_id   ~ if options & (1 << 7)
// +---------------+
// ~   source_sn   ~ if options & (1 << 8)
// +---------------+
//
// - if options & (1 << 0) then the payload is sliced
typedef struct {
    _z_bytes_t _source_id;
    _z_timestamp_t _tstamp;
    _z_zint_t _flags;
    _z_zint_t _source_sn;
    _z_encoding_t _encoding;
    uint8_t _kind;
} _z_data_info_t;
void _z_data_info_clear(_z_data_info_t *di);

/*------------------ Data Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|I|D|  DATA   |
// +-+-+-+---------+
// ~    ResKey     ~ if K==1 then keyexpr is string
// +---------------+
// ~    DataInfo   ~ if I==1
// +---------------+
// ~    Payload    ~
// +---------------+
//
// - if D==1 then the message can be dropped for congestion control reasons.
//
typedef struct {
    _z_data_info_t _info;
    _z_keyexpr_t _key;
    _z_payload_t _payload;
} _z_msg_data_t;
void _z_msg_clear_data(_z_msg_data_t *msg);

/*------------------ Unit Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|D|  UNIT   |
// +-+-+-+---------+
//
// - if D==1 then the message can be dropped for congestion control reasons.
//
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} _z_msg_unit_t;
void _z_msg_clear_unit(_z_msg_unit_t *unt);

/*------------------ Pull Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|N|F|  PULL   |
// +-+-+-+---------+
// ~    ResKey     ~ if K==1 then keyexpr is string
// +---------------+
// ~    pullid     ~
// +---------------+
// ~  max_samples  ~ if N==1
// +---------------+
//
typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _pull_id;
    _z_zint_t _max_samples;
} _z_msg_pull_t;
void _z_msg_clear_pull(_z_msg_pull_t *msg);

/*------------------ Query Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|B|T|  QUERY  |
// +-+-+-+---------+
// ~    ResKey     ~ if K==1 then keyexpr is string
// +---------------+
// ~   parameters   ~
// +---------------+
// ~      qid      ~
// +---------------+
// ~     target    ~ if T==1. Otherwise target = Z_TARGET_BEST_MATCHING
// +---------------+
// ~ consolidation ~
// +---------------+
// ~   QueryBody   ~ if B==1
// +---------------+
//
// where, QueryBody data structure is optionally included in Query messages
//
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// ~    DataInfo   ~
// +---------------+
// ~    Payload    ~
// +---------------+
// ```

typedef struct {
    _z_keyexpr_t _key;
    _z_zint_t _qid;
    char *_parameters;
    z_query_target_t _target;
    z_consolidation_mode_t _consolidation;
    _z_data_info_t _info;
    _z_payload_t _payload;
} _z_msg_query_t;
void _z_msg_clear_query(_z_msg_query_t *msg);

/*------------------ Zenoh Message ------------------*/
typedef union {
    _z_msg_declare_t _declare;
    _z_msg_data_t _data;
    _z_msg_query_t _query;
    _z_msg_pull_t _pull;
    _z_msg_unit_t _unit;
} _z_zenoh_body_t;
typedef struct {
    _z_zenoh_body_t _body;
    _z_attachment_t *_attachment;
    _z_reply_context_t *_reply_context;
    uint8_t _header;
} _z_zenoh_message_t;
void _z_msg_clear(_z_zenoh_message_t *m);
_Z_ELEM_DEFINE(_z_zenoh_message, _z_zenoh_message_t, _z_noop_size, _z_msg_clear, _z_noop_copy)
_Z_VEC_DEFINE(_z_zenoh_message, _z_zenoh_message_t)

/*------------------ Builders ------------------*/
_z_reply_context_t *_z_msg_make_reply_context(_z_zint_t qid, _z_bytes_t replier_id, _Bool is_final);
_z_declaration_t _z_msg_make_declaration_resource(_z_zint_t id, _z_keyexpr_t key);
_z_declaration_t _z_msg_make_declaration_forget_resource(_z_zint_t rid);
_z_declaration_t _z_msg_make_declaration_publisher(_z_keyexpr_t key);
_z_declaration_t _z_msg_make_declaration_forget_publisher(_z_keyexpr_t key);
_z_declaration_t _z_msg_make_declaration_subscriber(_z_keyexpr_t key, _z_subinfo_t subinfo);
_z_declaration_t _z_msg_make_declaration_forget_subscriber(_z_keyexpr_t key);
_z_declaration_t _z_msg_make_declaration_queryable(_z_keyexpr_t key, _z_zint_t complete, _z_zint_t distance);
_z_declaration_t _z_msg_make_declaration_forget_queryable(_z_keyexpr_t key);
_z_zenoh_message_t _z_msg_make_declare(_z_declaration_array_t declarations);
_z_zenoh_message_t _z_msg_make_data(_z_keyexpr_t key, _z_data_info_t info, _z_payload_t payload, _Bool can_be_dropped);
_z_zenoh_message_t _z_msg_make_unit(_Bool can_be_dropped);
_z_zenoh_message_t _z_msg_make_pull(_z_keyexpr_t key, _z_zint_t pull_id, _z_zint_t max_samples, _Bool is_final);
_z_zenoh_message_t _z_msg_make_query(_z_keyexpr_t key, char *parameters, _z_zint_t qid, z_query_target_t target,
                                     z_consolidation_mode_t consolidation, _z_value_t value);
_z_zenoh_message_t _z_msg_make_reply(_z_keyexpr_t key, _z_data_info_t info, _z_payload_t payload, _Bool can_be_dropped,
                                     _z_reply_context_t *rctx);

/*=============================*/
/*     Transport Messages      */
/*=============================*/
/*------------------ Scout Message ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// The SCOUT message can be sent at any point in time to solicit HELLO messages from matching parties.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|W|I|  SCOUT  |
// +-+-+-+-+-------+
// ~      what     ~ if W==1 -- Otherwise implicitly scouting for Routers
// +---------------+
//
// - if I==1 then the sender is asking for hello replies that contain a Zenoh ID.
//
typedef struct {
    z_whatami_t _what;
} _z_t_msg_scout_t;
void _z_t_msg_clear_scout(_z_t_msg_scout_t *msg);

/*------------------ Hello Message ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// The HELLO message is sent in any of the following three cases:
//     1) in response to a SCOUT message;
//     2) to (periodically) advertise (e.g., on multicast) the Peer and the locators it is reachable at;
//     3) in a already established session to update the corresponding peer on the new capabilities
//        (i.e., whatmai) and/or new set of locators (i.e., added or deleted).
// Locators are expressed as:
// <code>
//  udp/192.168.0.2:1234
//  tcp/192.168.0.2:1234
//  udp/239.255.255.123:5555
// <code>
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |L|W|I|  HELLO  |
// +-+-+-+-+-------+
// ~   zenoh_id    ~ if I==1
// +---------------+
// ~    whatami    ~ if W==1 -- Otherwise it is from a Router
// +---------------+
// ~    Locators   ~ if L==1 -- Otherwise src-address is the locator
// +---------------+
//
typedef struct {
    _z_bytes_t _zid;
    _z_locator_array_t _locators;
    z_whatami_t _whatami;
} _z_t_msg_hello_t;
void _z_t_msg_clear_hello(_z_t_msg_hello_t *msg);

/*------------------ Join Message ------------------*/
// # Join message
//
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// The JOIN message is sent on a multicast Locator to advertise the transport parameters.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |O|S|T|   JOIN  |
// +-+-+-+-+-------+
// ~             |Q~ if O==1
// +---------------+
// | v_maj | v_min | -- Protocol Version VMaj.VMin
// +-------+-------+
// ~    whatami    ~ -- Router, Peer or a combination of them
// +---------------+
// ~   zenoh_id    ~ -- PID of the sender of the JOIN message
// +---------------+
// ~     lease     ~ -- Lease period of the sender of the JOIN message(*)
// +---------------+
// ~ sn_resolution ~ if S==1(*) -- Otherwise 2^28 is assumed(**)
// +---------------+
// ~   [next_sn]   ~ (***)
// +---------------+
//
// - if Q==1 then the sender supports QoS.
//
// (*)   if T==1 then the lease period is expressed in seconds, otherwise in milliseconds
// (**)  if S==0 then 2^28 is assumed.
// (***) if Q==1 then 8 sequence numbers are present: one for each priority.
//       if Q==0 then only one sequence number is present.
//
typedef struct {
    _z_zint_t _reliable;
    _z_zint_t _best_effort;
} _z_coundit_sn_t;
typedef struct {
    union {
        _z_coundit_sn_t _plain;
        _z_coundit_sn_t _qos[Z_PRIORITIES_NUM];
    } _val;
    _Bool _is_qos;
} _z_conduit_sn_list_t;
typedef struct {
    _z_bytes_t _zid;
    _z_zint_t _options;
    _z_zint_t _lease;
    _z_zint_t _sn_resolution;
    _z_conduit_sn_list_t _next_sns;
    z_whatami_t _whatami;
    uint8_t _version;
} _z_t_msg_join_t;
void _z_t_msg_clear_join(_z_t_msg_join_t *msg);

/*------------------ Init Message ------------------*/
// # Init message
//
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// The INIT message is sent on a specific Locator to initiate a session with the peer associated
// with that Locator. The initiator MUST send an INIT message with the A flag set to 0.  If the
// corresponding peer deems appropriate to initialize a session with the initiator, the corresponding
// peer MUST reply with an INIT message with the A flag set to 1.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |O|S|A|   INIT  |
// +-+-+-+-+-------+
// ~             |Q~ if O==1
// +---------------+
// | v_maj | v_min | if A==0 -- Protocol Version VMaj.VMin
// +-------+-------+
// ~    whatami    ~ -- Client, Router, Peer or a combination of them
// +---------------+
// ~   zenoh_id    ~ -- PID of the sender of the INIT message
// +---------------+
// ~ sn_resolution ~ if S==1(*) -- Otherwise 2^28 is assumed(**)
// +---------------+
// ~     cookie    ~ if A==1
// +---------------+
//
// (*) if A==0 and S==0 then 2^28 is assumed.
//     if A==1 and S==0 then the agreed resolution is the one communicated by the initiator.
//
// - if Q==1 then the initiator/responder supports QoS.
//
typedef struct {
    _z_bytes_t _zid;
    _z_bytes_t _cookie;
    _z_zint_t _options;
    _z_zint_t _sn_resolution;
    z_whatami_t _whatami;
    uint8_t _version;
} _z_t_msg_init_t;
void _z_t_msg_clear_init(_z_t_msg_init_t *msg);

/*------------------ Open Message ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total lenght
//       in bytes of the message, resulting in the maximum lenght of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the lenght of a message must not exceed 65_535 bytes.
//
// The OPEN message is sent on a link to finally open an initialized session with the peer.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|T|A|   OPEN  |
// +-+-+-+-+-------+
// ~ lease_period  ~ -- Lease period of the sender of the OPEN message(*)
// +---------------+
// ~  initial_sn   ~ -- Initial SN proposed by the sender of the OPEN(**)
// +---------------+
// ~    cookie     ~ if A==0(*)
// +---------------+
//
// (*) if T==1 then the lease period is expressed in seconds, otherwise in milliseconds
// (**) the cookie MUST be the same received in the INIT message with A==1 from the corresponding peer
//
typedef struct {
    _z_zint_t _lease;
    _z_zint_t _initial_sn;
    _z_bytes_t _cookie;
} _z_t_msg_open_t;
void _z_t_msg_clear_open(_z_t_msg_open_t *msg);

/*------------------ Close Message ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// The CLOSE message is sent in any of the following two cases:
//     1) in response to an OPEN message which is not accepted;
//     2) at any time to arbitrarly close the session with the corresponding peer.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|K|I|  CLOSE  |
// +-+-+-+-+-------+
// ~   zenoh_id    ~  if I==1 -- PID of the target peer.
// +---------------+
// |     reason    |
// +---------------+
//
// - if K==0 then close the whole zenoh session.
// - if K==1 then close the transport link the CLOSE message was sent on (e.g., TCP socket) but
//           keep the whole session open. NOTE: the session will be automatically closed when
//           the session's lease period expires.
typedef struct {
    _z_bytes_t _zid;
    uint8_t _reason;
} _z_t_msg_close_t;
void _z_t_msg_clear_close(_z_t_msg_close_t *msg);

/*------------------ Sync Message ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// The SYNC message allows to signal the corresponding peer the sequence number of the next message
// to be transmitted on the reliable or best-effort channel. In the case of reliable channel, the
// peer can optionally include the number of unacknowledged messages. A SYNC sent on the reliable
// channel triggers the transmission of an ACKNACK message.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|C|R|  SYNC   |
// +-+-+-+-+-------+
// ~      sn       ~ -- Sequence number of the next message to be transmitted on this channel.
// +---------------+
// ~     count     ~ if R==1 && C==1 -- Number of unacknowledged messages.
// +---------------+
//
// - if R==1 then the SYNC concerns the reliable channel, otherwise the best-effort channel.
//
typedef struct {
    _z_zint_t _sn;
    _z_zint_t _count;
} _z_t_msg_sync_t;
void _z_t_msg_clear_sync(_z_t_msg_sync_t *msg);

/*------------------ AckNack Message ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// The ACKNACK messages is used on the reliable channel to signal the corresponding peer the last
// sequence number received and optionally a bitmask of the non-received messages.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|M| ACKNACK |
// +-+-+-+-+-------+
// ~      sn       ~
// +---------------+
// ~     mask      ~ if M==1
// +---------------+
//
typedef struct {
    _z_zint_t _sn;
    _z_zint_t _mask;
} _z_t_msg_ack_nack_t;
void _z_t_msg_clear_ack_nack(_z_t_msg_ack_nack_t *msg);

/*------------------ Keep Alive Message ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// The KEEP_ALIVE message can be sent periodically to avoid the expiration of the session lease
// period in case there are no messages to be sent.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|I| K_ALIVE |
// +-+-+-+-+-------+
// ~   zenoh_id    ~ if I==1 -- Peer ID of the KEEP_ALIVE sender.
// +---------------+
//
typedef struct {
    _z_bytes_t _zid;
} _z_t_msg_keep_alive_t;
void _z_t_msg_clear_keep_alive(_z_t_msg_keep_alive_t *msg);

/*------------------ PingPong Messages ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|P|  P_PONG |
// +-+-+-+-+-------+
// ~     hash      ~
// +---------------+
//
// - if P==1 then the message is Ping, otherwise is Pong.
//
typedef struct {
    _z_zint_t _hash;
} _z_t_msg_ping_pong_t;
void _z_t_msg_clear_ping_pong(_z_t_msg_ping_pong_t *msg);

/*------------------ Frame Message ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |E|F|R|  FRAME  |
// +-+-+-+-+-------+
// ~      SN       ~
// +---------------+
// ~  FramePayload ~ -- if F==1 then the payload is a fragment of a single Zenoh Message, a list of complete Zenoh
// Messages otherwise.
// +---------------+
//
// - if R==1 then the FRAME is sent on the reliable channel, best-effort otherwise.
// - if F==1 then the FRAME is a fragment.
// - if E==1 then the FRAME is the last fragment. E==1 is valid iff F==1.
//
// NOTE: Only one bit would be sufficient to signal fragmentation in a IP-like fashion as follows:
//         - if F==1 then this FRAME is a fragment and more fragment will follow;
//         - if F==0 then the message is the last fragment if SN-1 had F==1,
//           otherwise it's a non-fragmented message.
//       However, this would require to always perform a two-steps de-serialization: first
//       de-serialize the FRAME and then the Payload. This is due to the fact the F==0 is ambigous
//       w.r.t. detecting if the FRAME is a fragment or not before SN re-ordering has occured.
//       By using the F bit to only signal whether the FRAME is fragmented or not, it allows to
//       de-serialize the payload in one single pass when F==0 since no re-ordering needs to take
//       place at this stage. Then, the F bit is used to detect the last fragment during re-ordering.
//
typedef union {
    _z_payload_t _fragment;
    _z_zenoh_message_vec_t _messages;
} _z_frame_payload_t;
typedef struct {
    _z_frame_payload_t _payload;
    _z_zint_t _sn;
} _z_t_msg_frame_t;
void _z_t_msg_clear_frame(_z_t_msg_frame_t *msg, uint8_t header);

/*------------------ Transport Message ------------------*/
typedef union {
    _z_t_msg_scout_t _scout;
    _z_t_msg_hello_t _hello;
    _z_t_msg_join_t _join;
    _z_t_msg_init_t _init;
    _z_t_msg_open_t _open;
    _z_t_msg_close_t _close;
    _z_t_msg_sync_t _sync;
    _z_t_msg_ack_nack_t _ack_nack;
    _z_t_msg_keep_alive_t _keep_alive;
    _z_t_msg_ping_pong_t _ping_pong;
    _z_t_msg_frame_t _frame;
} _z_transport_body_t;
typedef struct {
    _z_transport_body_t _body;
    _z_attachment_t *_attachment;
    uint8_t _header;
} _z_transport_message_t;
void _z_t_msg_clear(_z_transport_message_t *msg);

/*------------------ Builders ------------------*/
_z_transport_message_t _z_t_msg_make_scout(z_whatami_t what, _Bool request_zid);
_z_transport_message_t _z_t_msg_make_hello(z_whatami_t whatami, _z_bytes_t zid, _z_locator_array_t locators);
_z_transport_message_t _z_t_msg_make_join(uint8_t version, z_whatami_t whatami, _z_zint_t lease,
                                          _z_zint_t sn_resolution, _z_bytes_t zid, _z_conduit_sn_list_t next_sns);
_z_transport_message_t _z_t_msg_make_init_syn(uint8_t version, z_whatami_t whatami, _z_zint_t sn_resolution,
                                              _z_bytes_t zid, _Bool is_qos);
_z_transport_message_t _z_t_msg_make_init_ack(uint8_t version, z_whatami_t whatami, _z_zint_t sn_resolution,
                                              _z_bytes_t zid, _z_bytes_t cookie, _Bool is_qos);
_z_transport_message_t _z_t_msg_make_open_syn(_z_zint_t lease, _z_zint_t initial_sn, _z_bytes_t cookie);
_z_transport_message_t _z_t_msg_make_open_ack(_z_zint_t lease, _z_zint_t initial_sn);
_z_transport_message_t _z_t_msg_make_close(uint8_t reason, _z_bytes_t zid, _Bool link_only);
_z_transport_message_t _z_t_msg_make_sync(_z_zint_t sn, _Bool is_reliable, _z_zint_t count);
_z_transport_message_t _z_t_msg_make_ack_nack(_z_zint_t sn, _z_zint_t mask);
_z_transport_message_t _z_t_msg_make_keep_alive(_z_bytes_t zid);
_z_transport_message_t _z_t_msg_make_ping(_z_zint_t hash);
_z_transport_message_t _z_t_msg_make_pong(_z_zint_t hash);
_z_transport_message_t _z_t_msg_make_frame(_z_zint_t sn, _z_frame_payload_t payload, _Bool is_reliable,
                                           _Bool is_fragment, _Bool is_final);
_z_transport_message_t _z_t_msg_make_frame_header(_z_zint_t sn, _Bool is_reliable, _Bool is_fragment, _Bool is_final);

/*------------------ Copy ------------------*/
// @TODO: implement the remaining copyers
void _z_t_msg_copy(_z_transport_message_t *clone, _z_transport_message_t *msg);
void _z_t_msg_copy_scout(_z_t_msg_scout_t *clone, _z_t_msg_scout_t *msg);
void _z_t_msg_copy_hello(_z_t_msg_hello_t *clone, _z_t_msg_hello_t *msg);
void _z_t_msg_copy_join(_z_t_msg_join_t *clone, _z_t_msg_join_t *msg);
void _z_t_msg_copy_init(_z_t_msg_init_t *clone, _z_t_msg_init_t *msg);
void _z_t_msg_copy_open(_z_t_msg_open_t *clone, _z_t_msg_open_t *msg);
void _z_t_msg_copy_close(_z_t_msg_close_t *clone, _z_t_msg_close_t *msg);
void _z_t_msg_copy_sync(_z_t_msg_sync_t *clone, _z_t_msg_sync_t *msg);
void _z_t_msg_copy_ack_nack(_z_t_msg_ack_nack_t *clone, _z_t_msg_ack_nack_t *msg);
void _z_t_msg_copy_keep_alive(_z_t_msg_keep_alive_t *clone, _z_t_msg_keep_alive_t *msg);
void _z_t_msg_copy_ping_pong(_z_t_msg_ping_pong_t *clone, _z_t_msg_ping_pong_t *msg);
void _z_t_msg_copy_frame(_z_t_msg_frame_t *clone, _z_t_msg_frame_t *msg, uint8_t header);

#endif /* ZENOH_PICO_PROTOCOL_MSG_H */
