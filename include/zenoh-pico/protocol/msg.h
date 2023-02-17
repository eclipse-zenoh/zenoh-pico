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
#include "zenoh-pico/protocol/ext.h"

#define _Z_DEFAULT_BATCH_SIZE 65535
#define _Z_DEFAULT_SIZET_SIZE 8  // In bytes

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
// Scout message flags:
//      I ZenohID          if I==1 then the ZenohID is present
//      Z Extensions       if Z==1 then Zenoh extensions are present
#define _Z_FLAG_SCOUT_I 0x08  // 1 << 3
#define _Z_FLAG_SCOUT_Z 0x80  // 1 << 7

// Hello message flags:
//      L Locators         if L==1 then Locators are present
//      Z Extensions       if Z==1 then Zenoh extensions are present
#define _Z_FLAG_HELLO_L 0x20  // 1 << 5
#define _Z_FLAG_HELLO_Z 0x80  // 1 << 7

// Join message flags:
//      T Lease period     if T==1 then the lease period is in seconds else in milliseconds
//      S Size params      if S==1 then size parameters are exchanged
//      Z Extensions       if Z==1 then Zenoh extensions are present
#define _Z_FLAG_JOIN_T 0x20  // 1 << 5
#define _Z_FLAG_JOIN_S 0x40  // 1 << 6
#define _Z_FLAG_JOIN_Z 0x80  // 1 << 7

// Init message flags:
//      A Ack              if A==1 then the message is an acknowledgment (aka InitAck), otherwise InitSyn
//      S Size params      if S==1 then size parameters are exchanged
//      Z Extensions       if Z==1 then Zenoh extensions are present
#define _Z_FLAG_INIT_A 0x20  // 1 << 5
#define _Z_FLAG_INIT_S 0x40  // 1 << 6
#define _Z_FLAG_INIT_Z 0x80  // 1 << 7

// Open message flags:
//      A Ack              if A==1 then the message is an acknowledgment (aka OpenAck), otherwise OpenSyn
//      T Lease period     if T==1 then the lease period is in seconds else in milliseconds
//      Z Extensions       if Z==1 then Zenoh extensions are present
#define _Z_FLAG_OPEN_A 0x20  // 1 << 5
#define _Z_FLAG_OPEN_T 0x40  // 1 << 6
#define _Z_FLAG_OPEN_Z 0x80  // 1 << 7

// Frame message flags:
//      R Reliable         if R==1 it concerns the reliable channel, else the best-effort channel
//      Z Extensions       if Z==1 then Zenoh extensions are present
#define _Z_FLAG_FRAME_R 0x20  // 1 << 5
#define _Z_FLAG_FRAME_Z 0x80  // 1 << 7

// Close message flags:
//      S Session Close   if S==1 Session close or S==0 Link close
//      Z Extensions       if Z==1 then Zenoh extensions are present
#define _Z_FLAG_CLOSE_S 0x20  // 1 << 5
#define _Z_FLAG_CLOSE_Z 0x80  // 1 << 7

/* Transport message flags */
#define _Z_FLAG_T_Z \
    0x20  // 1 << 5 | MixedSlices      if Z==1 then the payload contains a mix of raw and shm_info payload

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
void _z_msg_free(_z_zenoh_message_t **m);
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
                                     z_consolidation_mode_t consolidation, _z_value_t with_value);
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
// |Z|X|X|  SCOUT  |
// +-+-+-+---------+
// |    version    |
// +---------------+
// |zid_len|I| what| (#)(*)
// +-+-+-+-+-+-+-+-+
// ~      [u8]     ~ if Flag(I)==1 -- ZenohID
// +---------------+
//
// (#) ZID length. If Flag(I)==1 it indicates how many bytes are used for the ZenohID bytes.
//     A ZenohID is minimum 1 byte and maximum 16 bytes. Therefore, the actual lenght is computed as:
//         real_zid_len := 1 + zid_len
//
// (*) What. It indicates a bitmap of WhatAmI interests.
//    The valid bitflags are:
//    - 0b001: Router
//    - 0b010: Peer
//    - 0b100: Client
//
typedef struct {
    _z_bytes_t _zid;
    z_what_t _what;
    uint8_t _version;
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
// |Z|X|L|  HELLO  |
// +-+-+-+---------+
// |    version    |
// +---------------+
// |zid_len|X|X|wai| (*)
// +-+-+-+-+-+-+-+-+
// ~     [u8]      ~ -- ZenohID
// +---------------+
// ~   <utf8;z8>   ~ if Flag(L)==1 -- List of locators
// +---------------+
//
// (*) WhatAmI. It indicates the role of the zenoh node sending the HELLO message.
//    The valid WhatAmI values are:
//    - 0b00: Router
//    - 0b01: Peer
//    - 0b10: Client
//    - 0b11: Reserved
//
typedef struct {
    _z_bytes_t _zid;
    _z_locator_array_t _locators;
    z_whatami_t _whatami;
    uint8_t _version;
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
// ~ seq_num_res ~ if S==1(*) -- Otherwise 2^28 is assumed(**)
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
    _z_zint_t _lease;
    uint16_t _batch_size;
    _z_conduit_sn_list_t _next_sn;
    z_whatami_t _whatami;
    uint8_t _key_id_res;
    uint8_t _req_id_res;
    uint8_t _seq_num_res;
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
// Flags:
// - A: Ack          if A==0 then the message is an InitSyn else it is an InitAck
// - S: Size params  if S==1 then size parameters are exchanged
// - Z: Extensions   if Z==1 then zenoh extensions will follow.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|S|A|   INIT  |
// +-+-+-+---------+
// |    version    |
// +---------------+
// |zid_len|x|x|wai| (#)(*)
// +-------+-+-+---+
// ~      [u8]     ~ -- ZenohID of the sender of the INIT message
// +---------------+
// |x|x|kid|rid|fsn| \                -- SN/ID resolution (+)
// +---------------+  | if Flag(S)==1
// |      u16      |  |               -- Batch Size ($)
// |               | /
// +---------------+
// ~    <u8;z16>   ~ -- if Flag(A)==1 -- Cookie
// +---------------+
// ~   [InitExts]  ~ -- if Flag(Z)==1
// +---------------+
//
// If A==1 and S==0 then size parameters are (ie. S flag) are accepted.
//
// (*) WhatAmI. It indicates the role of the zenoh node sending the INIT
// message.
//    The valid WhatAmI values are:
//    - 0b00: Router
//    - 0b01: Peer
//    - 0b10: Client
//    - 0b11: Reserved
//
// (#) ZID length. It indicates how many bytes are used for the ZenohID bytes.
//     A ZenohID is minimum 1 byte and maximum 16 bytes. Therefore, the actual
//     lenght is computed as:
//         real_zid_len := 1 + zid_len
//
// (+) Sequence Number/ID resolution. It indicates the resolution and
// consequently the wire overhead
//     of various SN and ID in Zenoh.
//     - fsn: frame/fragment sequence number resolution. Used in Frame/Fragment
//     messages.
//     - rid: request ID resolution. Used in Request/Response messages.
//     - kid: key expression ID resolution. Used in Push/Request/Response
//     messages. The valid SN/ID resolution values are:
//     - 0b00: 8 bits
//     - 0b01: 16 bits
//     - 0b10: 32 bits
//     - 0b11: 64 bits
//
// ($) Batch Size. It indicates the maximum size of a batch the sender of the
//
typedef struct {
    _z_bytes_t _zid;
    _z_bytes_t _cookie;
    uint16_t _batch_size;
    z_whatami_t _whatami;
    uint8_t _key_id_res;
    uint8_t _req_id_res;
    uint8_t _seq_num_res;
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
// Flags:
// - A Ack           if A==1 then the message is an acknowledgment (aka OpenAck), otherwise OpenSyn
// - T Lease period  if T==1 then the lease period is in seconds else in milliseconds
// - Z Extensions    if Z==1 then Zenoh extensions are present
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|T|A|   OPEN  |
// +-+-+-+---------+
// %     lease     % -- Lease period of the sender of the OPEN message
// +---------------+
// %  initial_sn   % -- Initial SN proposed by the sender of the OPEN(*)
// +---------------+
// ~    <u8;z16>   ~ if Flag(A)==0 (**) -- Cookie
// +---------------+
// ~   [OpenExts]  ~ if Flag(Z)==1
// +---------------+
//
// (*)     The initial sequence number MUST be compatible with the sequence number resolution agreed in the
//         [`super::InitSyn`]-[`super::InitAck`] message exchange
// (**)    The cookie MUST be the same received in the [`super::InitAck`]from the corresponding zenoh node
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
// Flags:
// - S: Session Close  if S==1 Session close or S==0 Link close
// - X: Reserved
// - Z: Extensions     if Z==1 then zenoh extensions will follow.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|X|S|  CLOSE  |
// +-+-+-+---------+
// |    Reason     |
// +---------------+
// ~  [CloseExts]  ~ if Flag(Z)==1
// +---------------+
//
typedef struct {
    uint8_t _reason;
} _z_t_msg_close_t;
void _z_t_msg_clear_close(_z_t_msg_close_t *msg);

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
// Flags:
// - X: Reserved
// - X: Reserved
// - Z: Extensions     If Z==1 then Zenoh extensions will follow.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|X|X| KALIVE  |
// +-+-+-+---------+
// ~  [KAliveExts] ~ if Flag(Z)==1
// +---------------+
//
typedef struct {
    uint8_t __dummy;  // Just to avoid empty structures that might cause undefined behavior
} _z_t_msg_keep_alive_t;
void _z_t_msg_clear_keep_alive(_z_t_msg_keep_alive_t *msg);

/*------------------ Frame Message ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// Flags:
// - R: Reliable       If R==1 it concerns the reliable channel, else the best-effort channel
// - X: Reserved
// - Z: Extensions     If Z==1 then zenoh extensions will follow.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|X|R|  FRAME  |
// +-+-+-+---------+
// %    seq num    %
// +---------------+
// ~  [FrameExts]  ~ if Flag(Z)==1
// +---------------+
// ~  [NetworkMsg] ~
// +---------------+
//
// - if R==1 then the FRAME is sent on the reliable channel, best-effort otherwise.
//
typedef struct {
    _z_zenoh_message_vec_t _messages;
    _z_zint_t _sn;
} _z_t_msg_frame_t;
void _z_t_msg_clear_frame(_z_t_msg_frame_t *msg);

/*------------------ Transport Message ------------------*/
typedef union {
    _z_t_msg_scout_t _scout;
    _z_t_msg_hello_t _hello;
    _z_t_msg_join_t _join;
    _z_t_msg_init_t _init;
    _z_t_msg_open_t _open;
    _z_t_msg_close_t _close;
    _z_t_msg_keep_alive_t _keep_alive;
    _z_t_msg_frame_t _frame;
} _z_transport_body_t;

typedef struct {
    _z_transport_body_t _body;
    _z_attachment_t *_attachment;
    _z_msg_ext_vec_t _extensions;
    uint8_t _header;
} _z_transport_message_t;
void _z_t_msg_clear(_z_transport_message_t *msg);

/*------------------ Builders ------------------*/
_z_transport_message_t _z_t_msg_make_scout(z_what_t what, _z_bytes_t zid);
_z_transport_message_t _z_t_msg_make_hello(z_whatami_t whatami, _z_bytes_t zid, _z_locator_array_t locators);
_z_transport_message_t _z_t_msg_make_join(z_whatami_t whatami, _z_zint_t lease, _z_bytes_t zid,
                                          _z_conduit_sn_list_t next_sn);
_z_transport_message_t _z_t_msg_make_init_syn(z_whatami_t whatami, _z_bytes_t zid);
_z_transport_message_t _z_t_msg_make_init_ack(z_whatami_t whatami, _z_bytes_t zid, _z_bytes_t cookie);
_z_transport_message_t _z_t_msg_make_open_syn(_z_zint_t lease, _z_zint_t initial_sn, _z_bytes_t cookie);
_z_transport_message_t _z_t_msg_make_open_ack(_z_zint_t lease, _z_zint_t initial_sn);
_z_transport_message_t _z_t_msg_make_close(uint8_t reason, _Bool link_only);
_z_transport_message_t _z_t_msg_make_keep_alive(void);
_z_transport_message_t _z_t_msg_make_frame(_z_zint_t sn, _z_zenoh_message_vec_t messages, _Bool is_reliable);
_z_transport_message_t _z_t_msg_make_frame_header(_z_zint_t sn, _Bool is_reliable);

/*------------------ Copy ------------------*/
void _z_t_msg_copy(_z_transport_message_t *clone, _z_transport_message_t *msg);
void _z_t_msg_copy_scout(_z_t_msg_scout_t *clone, _z_t_msg_scout_t *msg);
void _z_t_msg_copy_hello(_z_t_msg_hello_t *clone, _z_t_msg_hello_t *msg);
void _z_t_msg_copy_join(_z_t_msg_join_t *clone, _z_t_msg_join_t *msg);
void _z_t_msg_copy_init(_z_t_msg_init_t *clone, _z_t_msg_init_t *msg);
void _z_t_msg_copy_open(_z_t_msg_open_t *clone, _z_t_msg_open_t *msg);
void _z_t_msg_copy_close(_z_t_msg_close_t *clone, _z_t_msg_close_t *msg);
void _z_t_msg_copy_keep_alive(_z_t_msg_keep_alive_t *clone, _z_t_msg_keep_alive_t *msg);
void _z_t_msg_copy_frame(_z_t_msg_frame_t *clone, _z_t_msg_frame_t *msg);

#endif /* ZENOH_PICO_PROTOCOL_MSG_H */
