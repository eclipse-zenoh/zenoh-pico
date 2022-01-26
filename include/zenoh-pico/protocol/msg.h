/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
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

#ifndef ZENOH_PICO_PROTOCOL_MSG_H
#define ZENOH_PICO_PROTOCOL_MSG_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/collections/array.h"
#include "zenoh-pico/collections/string.h"

#define _ZN_DECLARE_CLEAR(layer, name) \
    void _zn_##layer##_msg_clear_##name(_zn_##name##_t *m, uint8_t header)

#define _ZN_DECLARE_CLEAR_NOH(layer, name) \
    void _zn_##layer##_msg_clear_##name(_zn_##name##_t *m)

// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
#define _ZN_MSG_LEN_ENC_SIZE 2

/*=============================*/
/*         Message IDs         */
/*=============================*/
/* Transport Messages */
#define _ZN_MID_SCOUT 0x01
#define _ZN_MID_HELLO 0x02
#define _ZN_MID_INIT 0x01  // Note: for unicast only
#define _ZN_MID_OPEN 0x02  // Note: for unicast only
#define _ZN_MID_JOIN 0x03  // Note: for multicast only
#define _ZN_MID_CLOSE 0x05
#define _ZN_MID_SYNC 0x06
#define _ZN_MID_ACK_NACK 0x07
#define _ZN_MID_KEEP_ALIVE 0x08
#define _ZN_MID_PING_PONG 0x09
#define _ZN_MID_FRAME 0x0a
/* Zenoh Messages */
#define _ZN_MID_DECLARE 0x0b
#define _ZN_MID_DATA 0x0c
#define _ZN_MID_QUERY 0x0d
#define _ZN_MID_PULL 0x0e
#define _ZN_MID_UNIT 0x0f
#define _ZN_MID_LINK_STATE_LIST 0x10
/* Message decorators */
#define _ZN_MID_PRIORITY 0x1c
#define _ZN_MID_ROUTING_CONTEXT 0x1d
#define _ZN_MID_REPLY_CONTEXT 0x1e
#define _ZN_MID_ATTACHMENT 0x1f

/*=============================*/
/*        Message flags        */
/*=============================*/
// Scouting message flags:
//      I ZenohID          if I==1 then the ZenohID is present
//      L Locators         if L==1 then Locators are present
//      O Options          if O==1 then Options are present
//      Z Zenoh properties if Z==1 then Zenoh properties are present
//      X None             Non-attributed flags are set to zero

#define _ZN_FLAG_HDR_SCOUT_I    0x20  // 1 << 5
#define _ZN_FLAG_HDR_SCOUT_Z    0x80  // 1 << 7

#define _ZN_FLAG_HDR_HELLO_L    0x20  // 1 << 5
#define _ZN_FLAG_HDR_HELLO_Z    0x80  // 1 << 7

#define _ZN_FLAG_S_O            0x80  // 1 << 7
#define _ZN_FLAG_S_X            0x00  // 0

// Transport message flags:
//      A Ack              if A==1 then the message is an acknowledgment
//      E End              if E==1 then it is the last FRAME fragment
//      F Fragment         if F==1 then the FRAME is a fragment
//      O Options          if O==1 then the next byte is an additional option
//      Q QoS              if Q==1 then the sender of the message supports QoS
//      R Reliable         if R==1 then it concerns the reliable channel, best-effort otherwise
//      T TimeRes          if T==1 then the time resolution is in seconds
//      Z Zenoh properties if Z==1 then Zenoh properties are present
//      X None             Non-attributed flags are set to zero

#define _ZN_FLAG_HDR_INIT_A     0x20  // 1 << 5
#define _ZN_FLAG_HDR_INIT_Z     0x80  // 1 << 7

#define _ZN_FLAG_HDR_OPEN_A     0x20  // 1 << 5
#define _ZN_FLAG_HDR_OPEN_T     0x40  // 1 << 6
#define _ZN_FLAG_HDR_OPEN_Z     0x80  // 1 << 7

#define _ZN_FLAG_HDR_JOIN_T     0x40  // 1 << 6
#define _ZN_FLAG_HDR_JOIN_Z     0x80  // 1 << 7

#define _ZN_FLAG_T_O            0x80  // 1 << 7
#define _ZN_FLAG_T_X            0x00  // 0

/* Transport message flags */
#define _ZN_FLAG_T_A 0x20  // 1 << 5 | Ack              if A==1 then the message is an acknowledgment
#define _ZN_FLAG_T_C 0x40  // 1 << 6 | Count            if C==1 then number of unacknowledged messages is present
#define _ZN_FLAG_T_E 0x80  // 1 << 7 | End              if E==1 then it is the last FRAME fragment
#define _ZN_FLAG_T_F 0x40  // 1 << 6 | Fragment         if F==1 then the FRAME is a fragment
#define _ZN_FLAG_T_I 0x20  // 1 << 5 | PeerID           if I==1 then the PeerID is present or requested
#define _ZN_FLAG_T_K 0x40  // 1 << 6 | CloseLink        if K==1 then close the transport link only
#define _ZN_FLAG_T_L 0x80  // 1 << 7 | Locators         if L==1 then Locators are present
#define _ZN_FLAG_T_M 0x20  // 1 << 5 | Mask             if M==1 then a Mask is present
#define _ZN_FLAG_T_O 0x80  // 1 << 7 | Options          if O==1 then Options are present
#define _ZN_FLAG_T_P 0x20  // 1 << 5 | PingOrPong       if P==1 then the message is Ping, otherwise is Pong
#define _ZN_FLAG_T_R 0x20  // 1 << 5 | Reliable         if R==1 then it concerns the reliable channel, best-effort otherwise
#define _ZN_FLAG_T_S 0x40  // 1 << 6 | SN Resolution    if S==1 then the SN Resolution is present
#define _ZN_FLAG_T_T1 0x20 // 1 << 5 | TimeRes          if T==1 then the time resolution is in seconds
#define _ZN_FLAG_T_T2 0x40 // 1 << 6 | TimeRes          if T==1 then the time resolution is in seconds
#define _ZN_FLAG_T_W 0x40  // 1 << 6 | WhatAmI          if W==1 then WhatAmI is indicated
#define _ZN_FLAG_T_Z 0x20  // 1 << 5 | MixedSlices      if Z==1 then the payload contains a mix of raw and shm_info payload
#define _ZN_FLAG_T_X 0x00  // Unused flags are set to zero

/* Zenoh message flags */
#define _ZN_FLAG_Z_D 0x20 // 1 << 5 | Dropping          if D==1 then the message can be dropped
#define _ZN_FLAG_Z_F 0x20 // 1 << 5 | Final             if F==1 then this is the final message (e.g., ReplyContext, Pull)
#define _ZN_FLAG_Z_I 0x40 // 1 << 6 | DataInfo          if I==1 then DataInfo is present
#define _ZN_FLAG_Z_K 0x80 // 1 << 7 | ResourceKey       if K==1 then reskey is string
#define _ZN_FLAG_Z_N 0x40 // 1 << 6 | MaxSamples        if N==1 then the MaxSamples is indicated
#define _ZN_FLAG_Z_P 0x80 // 1 << 7 | Period            if P==1 then a period is present
#define _ZN_FLAG_Z_Q 0x40 // 1 << 6 | QueryableKind     if Q==1 then the queryable kind is present
#define _ZN_FLAG_Z_R 0x20 // 1 << 5 | Reliable          if R==1 then it concerns the reliable channel, best-effort otherwise
#define _ZN_FLAG_Z_S 0x40 // 1 << 6 | SubMode           if S==1 then the declaration SubMode is indicated
#define _ZN_FLAG_Z_T 0x20 // 1 << 5 | QueryTarget       if T==1 then the query target is present
#define _ZN_FLAG_Z_X 0x00 // Unused flags are set to zero

/* Init option flags */
#define _ZN_OPT_INIT_QOS 0x01 // 1 << 0 | QoS       if QOS==1 then the session supports QoS
#define _ZN_OPT_JOIN_QOS 0x01 // 1 << 0 | QoS       if QOS==1 then the session supports QoS

/*=============================*/
/*       Message header        */
/*=============================*/
#define _ZN_MID_MASK 0x1f
#define _ZN_FLAGS_MASK 0xe0

/*=============================*/
/*       Message helpers       */
/*=============================*/
#define _ZN_MID(h) (_ZN_MID_MASK & h)
#define _ZN_FLAGS(h) (_ZN_FLAGS_MASK & h)
#define _ZN_HAS_FLAG(h, f) ((h & f) != 0)
#define _ZN_SET_FLAG(h, f) (h |= f)

/*=============================*/
/*       Declaration IDs       */
/*=============================*/
#define _ZN_DECL_RESOURCE 0x01
#define _ZN_DECL_PUBLISHER 0x02
#define _ZN_DECL_SUBSCRIBER 0x03
#define _ZN_DECL_QUERYABLE 0x04
#define _ZN_DECL_FORGET_RESOURCE 0x11
#define _ZN_DECL_FORGET_PUBLISHER 0x12
#define _ZN_DECL_FORGET_SUBSCRIBER 0x13
#define _ZN_DECL_FORGET_QUERYABLE 0x14

/*=============================*/
/*        Close reasons        */
/*=============================*/
#define _ZN_CLOSE_GENERIC 0x00
#define _ZN_CLOSE_UNSUPPORTED 0x01
#define _ZN_CLOSE_INVALID 0x02
#define _ZN_CLOSE_MAX_TRANSPORTS 0x03
#define _ZN_CLOSE_MAX_LINKS 0x04
#define _ZN_CLOSE_EXPIRED 0x05

/*=============================*/
/*       DataInfo flags        */
/*=============================*/
#define _ZN_DATA_INFO_SLICED 0x01 // 1 << 0
#define _ZN_DATA_INFO_KIND 0x02   // 1 << 1
#define _ZN_DATA_INFO_ENC 0x04    // 1 << 2
#define _ZN_DATA_INFO_TSTAMP 0x08 // 1 << 3
// Reserved: bits 4-6
#define _ZN_DATA_INFO_SRC_ID 0x80  // 1 << 7
#define _ZN_DATA_INFO_SRC_SN 0x100 // 1 << 8
#define _ZN_DATA_INFO_RTR_ID 0x200 // 1 << 9
#define _ZN_DATA_INFO_RTR_SN 0x400 // 1 << 10

/*------------------ Payload field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// ~    Length     ~
// +---------------+
// ~    Buffer     ~
// +---------------+
//
typedef z_bytes_t _zn_payload_t;
void _zn_payload_clear(_zn_payload_t *p);

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
typedef struct
{
    _zn_payload_t payload;
    uint8_t header;
} _zn_attachment_t;
void _zn_t_msg_clear_attachment(_zn_attachment_t *a);

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
// ~  replier_kind ~ if F==0
// +---------------+
// ~   replier_id  ~ if F==0
// +---------------+
//
// - if F==1 then the message is a REPLY_FINAL
//
typedef struct
{
    z_zint_t qid;
    z_zint_t replier_kind;
    z_bytes_t replier_id;
    uint8_t header;
} _zn_reply_context_t;
void _zn_z_msg_clear_reply_context(_zn_reply_context_t *rc);

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
// typdef struct { ... } zn_reskey_t; is defined in zenoh/net/types.h
void _zn_reskey_clear(zn_reskey_t *rk);

/*------------------ Resource Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|   RES   |
// +---------------+
// ~      RID      ~
// +---------------+
// ~    ResKey     ~ if K==1 then reskey is string
// +---------------+
//
typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
} _zn_res_decl_t;
void _zn_z_msg_clear_declaration_resource(_zn_res_decl_t *dcl);

/*------------------ Forget Resource Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|X|  F_RES  |
// +---------------+
// ~      RID      ~
// +---------------+
//
typedef struct
{
    z_zint_t rid;
} _zn_forget_res_decl_t;
void _zn_z_msg_clear_declaration_forget_resource(_zn_forget_res_decl_t *dcl);

/*------------------ Publisher Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|   PUB   |
// +---------------+
// ~    ResKey     ~ if K==1 then reskey is string
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
} _zn_pub_decl_t;
void _zn_z_msg_clear_declaration_publisher(_zn_pub_decl_t *dcl);

/*------------------ Forget Publisher Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|  F_PUB  |
// +---------------+
// ~    ResKey     ~ if K==1 then reskey is string
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
} _zn_forget_pub_decl_t;
void _zn_z_msg_clear_declaration_forget_publisher(_zn_forget_pub_decl_t *dcl);

/*------------------ SubInfo Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// |P|X|X|  SM_ID  |
// +---------------+
// ~    Period     ~ if P==1
// +---------------+
//
// typdef struct { ... } zn_subinfo_t; is defined in net/types.h
void _zn_subinfo_clear(zn_subinfo_t *si);

/*------------------ Subscriber Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|S|R|   SUB   |
// +---------------+
// ~    ResKey     ~ if K==1 then reskey is string
// +---------------+
// ~    SubInfo    ~ if S==1. Otherwise: SubMode=Push
// +---------------+
//
// - if R==1 then the subscription is reliable, best-effort otherwise.
//
typedef struct
{
    zn_reskey_t key;
    zn_subinfo_t subinfo;
} _zn_sub_decl_t;
void _zn_z_msg_clear_declaration_subscriber(_zn_sub_decl_t *dcl);

/*------------------ Forget Subscriber Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|  F_SUB  |
// +---------------+
// ~    ResKey     ~ if K==1 then reskey is string
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
} _zn_forget_sub_decl_t;
void _zn_z_msg_clear_declaration_forget_subscriber(_zn_forget_sub_decl_t *dcl);

/*------------------ Queryable Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|Q|X|  QABLE  |
// +---------------+
// ~     ResKey    ~ if K==1 then reskey is string
// +---------------+
// ~     Kind      ~
// +---------------+
// ~   QablInfo    ~ if Q==1
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
    z_zint_t kind;
    z_zint_t complete;
    z_zint_t distance;
} _zn_qle_decl_t;
void _zn_z_msg_clear_declaration_queryable(_zn_qle_decl_t *dcl);

/*------------------ Forget Queryable Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X| F_QABLE |
// +---------------+
// ~    ResKey     ~ if K==1 then reskey is string
// +---------------+
// ~     Kind      ~
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
    z_zint_t kind;
} _zn_forget_qle_decl_t;
void _zn_z_msg_clear_declaration_forget_queryable(_zn_forget_qle_decl_t *dcl);

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
typedef struct
{
    union
    {
        _zn_res_decl_t res;
        _zn_forget_res_decl_t forget_res;
        _zn_pub_decl_t pub;
        _zn_forget_pub_decl_t forget_pub;
        _zn_sub_decl_t sub;
        _zn_forget_sub_decl_t forget_sub;
        _zn_qle_decl_t qle;
        _zn_forget_qle_decl_t forget_qle;
    } body;
    uint8_t header;
} _zn_declaration_t;

void _zn_z_msg_clear_declaration(_zn_declaration_t *dcl);
_Z_ELEM_DEFINE(_zn_declaration, _zn_declaration_t, _zn_noop_size, _zn_z_msg_clear_declaration, _zn_noop_copy)
_Z_ARRAY_DEFINE(_zn_declaration, _zn_declaration_t)

typedef struct
{
    _zn_declaration_array_t declarations;
} _zn_declare_t;
void _zn_z_msg_clear_declare(_zn_declare_t *dcl);

/*------------------ Timestamp Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// ~     Time      ~  Encoded as z_zint_t
// +---------------+
// ~      ID       ~
// +---------------+
//
// typdef struct { ... } z_timestamp_t; is defined in zenoh/types.h
void z_timestamp_clear(z_timestamp_t *ts);

/*------------------ Data Info Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// ~    options    ~
// +---------------+
// ~      kind     ~ if options & (1 << 0)
// +---------------+
// ~   encoding    ~ if options & (1 << 1)
// +---------------+
// ~   timestamp   ~ if options & (1 << 2)
// +---------------+
// ~   source_id   ~ if options & (1 << 7)
// +---------------+
// ~   source_sn   ~ if options & (1 << 8)
// +---------------+
// ~first_router_id~ if options & (1 << 9)
// +---------------+
// ~first_router_sn~ if options & (1 << 10)
// +---------------+
//
// - if options & (1 << 5) then the payload is sliced
typedef struct
{
    z_zint_t flags;
    z_zint_t kind;
    z_encoding_t encoding;
    z_timestamp_t tstamp;
    z_bytes_t source_id;
    z_zint_t source_sn;
    z_bytes_t first_router_id;
    z_zint_t first_router_sn;
} _zn_data_info_t;
void _zn_data_info_clear(_zn_data_info_t *di);

/*------------------ Data Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|I|D|  DATA   |
// +-+-+-+---------+
// ~    ResKey     ~ if K==1 then reskey is string
// +---------------+
// ~    DataInfo   ~ if I==1
// +---------------+
// ~    Payload    ~
// +---------------+
//
// - if D==1 then the message can be dropped for congestion control reasons.
//
typedef struct
{
    zn_reskey_t key;
    _zn_data_info_t info;
    _zn_payload_t payload;
} _zn_data_t;
void _zn_z_msg_clear_data(_zn_data_t *msg);

/*------------------ Unit Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|D|  UNIT   |
// +-+-+-+---------+
//
// - if D==1 then the message can be dropped for congestion control reasons.
//
typedef struct
{
} _zn_unit_t;
void _zn_z_msg_clear_unit(_zn_unit_t *unt);

/*------------------ Pull Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|N|F|  PULL   |
// +-+-+-+---------+
// ~    ResKey     ~ if K==1 then reskey is string
// +---------------+
// ~    pullid     ~
// +---------------+
// ~  max_samples  ~ if N==1
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
    z_zint_t pull_id;
    z_zint_t max_samples;
} _zn_pull_t;
void _zn_z_msg_clear_pull(_zn_pull_t *msg);

/*------------------ Query Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|T|  QUERY  |
// +-+-+-+---------+
// ~    ResKey     ~ if K==1 then reskey is string
// +---------------+
// ~   predicate   ~
// +---------------+
// ~      qid      ~
// +---------------+
// ~     target    ~ if T==1. Otherwise target.kind = ZN_QUERYABLE_ALL_KINDS, target.tag = zn_target_t_BEST_MATCHING
// +---------------+
// ~ consolidation ~
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
    z_str_t predicate;
    z_zint_t qid;
    zn_query_target_t target;
    zn_query_consolidation_t consolidation;
} _zn_query_t;
void _zn_z_msg_clear_query(_zn_query_t *msg);

/*------------------ Zenoh Message ------------------*/
typedef union
{
    _zn_declare_t declare;
    _zn_data_t data;
    _zn_query_t query;
    _zn_pull_t pull;
    _zn_unit_t unit;
} _zn_zenoh_body_t;
typedef struct
{
    _zn_attachment_t *attachment;
    _zn_reply_context_t *reply_context;
    _zn_zenoh_body_t body;
    uint8_t header;
} _zn_zenoh_message_t;
void _zn_z_msg_clear(_zn_zenoh_message_t *m);
_Z_ELEM_DEFINE(_zn_zenoh_message, _zn_zenoh_message_t, _zn_noop_size, _zn_z_msg_clear, _zn_noop_copy)
_Z_VEC_DEFINE(_zn_zenoh_message, _zn_zenoh_message_t)

/*------------------ Builders ------------------*/
_zn_reply_context_t *_zn_z_msg_make_reply_context(z_zint_t qid, z_bytes_t replier_id, z_zint_t replier_kind, int is_final);
_zn_declaration_t _zn_z_msg_make_declaration_resource(z_zint_t id, zn_reskey_t key);
_zn_declaration_t _zn_z_msg_make_declaration_forget_resource(z_zint_t rid);
_zn_declaration_t _zn_z_msg_make_declaration_publisher(zn_reskey_t key);
_zn_declaration_t _zn_z_msg_make_declaration_forget_publisher(zn_reskey_t key);
_zn_declaration_t _zn_z_msg_make_declaration_subscriber(zn_reskey_t key, zn_subinfo_t subinfo);
_zn_declaration_t _zn_z_msg_make_declaration_forget_subscriber(zn_reskey_t key);
_zn_declaration_t _zn_z_msg_make_declaration_queryable(zn_reskey_t key, z_zint_t kind, z_zint_t complete, z_zint_t distance);
_zn_declaration_t _zn_z_msg_make_declaration_forget_queryable(zn_reskey_t key, z_zint_t kind);
_zn_zenoh_message_t _zn_z_msg_make_declare(_zn_declaration_array_t declarations);
_zn_zenoh_message_t _zn_z_msg_make_data(zn_reskey_t key, _zn_data_info_t info, _zn_payload_t payload, int can_be_dropped);
_zn_zenoh_message_t _zn_z_msg_make_unit(int can_be_dropped);
_zn_zenoh_message_t _zn_z_msg_make_pull(zn_reskey_t key, z_zint_t pull_id, z_zint_t max_samples, int is_final);
_zn_zenoh_message_t _zn_z_msg_make_query(zn_reskey_t key, z_str_t predicate, z_zint_t qid, zn_query_target_t target, zn_query_consolidation_t consolidation);
_zn_zenoh_message_t _zn_z_msg_make_reply(zn_reskey_t key, _zn_data_info_t info, _zn_payload_t payload, int can_be_dropped, _zn_reply_context_t *rctx);

/*=============================*/
/*     Transport Messages      */
/*=============================*/
/*------------------ Scout Message ------------------*/
// The SCOUT message can be sent at any point in time to solicit HELLO messages from matching parties.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|X|I|  SCOUT  |
// +-+-+-+---------+
// |    version    |
// +---------------+
// |X|X|X|X|X| what| (*)
// +---+-------+---+
// ~     <u8>      ~ if Flag(I)==1 -- ZenohID
// +---------------+
//
typedef struct
{
    uint8_t version;
    uint8_t what;
    z_bytes_t zid;
} _zn_scout_t;
void _zn_t_msg_clear_scout(_zn_scout_t *msg, uint8_t header);

/*------------------ Hello Message ------------------*/
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
// |X|X|X|X|X|X|wai| (*)
// +-+-+-+-+-+-+-+-+
// ~     <u8>      ~ -- ZenohID
// +---------------+
// ~   <<utf8>>    ~ if Flag(L)==1 -- List of locators
// +---------------+
//
typedef struct
{
    uint8_t version;
    uint8_t whatami;
    z_bytes_t zid;
    _zn_locator_array_t locators;
} _zn_hello_t;
void _zn_t_msg_clear_hello(_zn_hello_t *msg, uint8_t header);

/*------------------ Join Message ------------------*/
// The JOIN message is sent on a multicast Locator to advertise the transport parameters.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|T|X|   JOIN  |
// +-+-+-+---------+
// |    version    |
// +---------------+
// |X|X|X|sn_bs|wai| (#)(*)
// +-----+-----+---+
// ~     <u8>      ~ -- ZenohID of the sender of the JOIN message
// +---------------+
// %     lease     % -- Lease period of the sender of the JOIN message
// +---------------+
// ~    next_sn    ~  -- Next conduit sequence number
// +---------------+
//
// (#)(*) See INIT message
//
typedef struct
{
    z_zint_t reliable;
    z_zint_t best_effort;
} _zn_conduit_sn_t;

typedef struct
{
    uint8_t version;
    uint8_t whatami;
    uint8_t sn_bs;
    z_bytes_t zid;
    z_zint_t lease;
    _zn_conduit_sn_t next_sn;
} _zn_join_t;
void _zn_t_msg_clear_join(_zn_join_t *msg, uint8_t header);

/*------------------ Init Message ------------------*/
// The INIT message is sent on a specific Locator to initiate a transport with the peer associated
// with that Locator. The initiator MUST send an INIT message with the A flag set to 0. If the
// corresponding peer deems appropriate to initialize a transport with the initiator, the corresponding
// peer MUST reply with an INIT message with the A flag set to 1.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|X|A|   INIT  |
// +-+-+-+---------+
// |    version    |
// +---------------+
// |X|X|X|sn_bs|wai| (#)(*)
// +-+-+-+-----+---+
// ~      <u8>     ~ -- ZenohID of the sender of the INIT message
// +---------------+
// ~      <u8>     ~ if Flag(A)==1 -- Cookie
// +---------------+
//
// (*) WhatAmI. It indicates the role of the zenoh node sending the INIT message.
//    The valid WhatAmI values are:
//    - 0b00: Router
//    - 0b01: Peer
//    - 0b10: Client
//    - 0b11: Reserved
//
typedef struct
{
    uint8_t version;
    uint8_t whatami;
    uint8_t sn_bs;
    z_bytes_t zid;
    z_bytes_t cookie;
} _zn_init_t;
void _zn_t_msg_clear_init(_zn_init_t *msg, uint8_t header);

/*------------------ Open Message ------------------*/
// The OPEN message is sent on a link to finally open an initialized session with the peer.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |Z|T|A|   OPEN  |
// +-+-+-+---------+
// %     lease     % -- Lease period of the sender of the OPEN message
// +---------------+
// %  initial_sn   % -- Initial SN proposed by the sender of the OPEN(*)
// +---------------+
// ~     <u8>      ~ if Flag(A)==0 (**) -- Cookie
// +---------------+
//
// (*)     The initial sequence number MUST be compatible with the sequence number resolution agreed in the
//         [`super::InitSyn`]-[`super::InitAck`] message exchange
// (**)    The cookie MUST be the same received in the [`super::InitAck`]from the corresponding zenoh node
//
typedef struct
{
    z_zint_t lease;
    z_zint_t initial_sn;
    z_bytes_t cookie;
} _zn_open_t;
void _zn_t_msg_clear_open(_zn_open_t *msg, uint8_t header);

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
// ~    peer_id    ~  if I==1 -- PID of the target peer.
// +---------------+
// |     reason    |
// +---------------+
//
// - if K==0 then close the whole zenoh session.
// - if K==1 then close the transport link the CLOSE message was sent on (e.g., TCP socket) but
//           keep the whole session open. NOTE: the session will be automatically closed when
//           the session's lease period expires.
typedef struct
{
    z_bytes_t pid;
    uint8_t reason;
} _zn_close_t;
void _zn_t_msg_clear_close(_zn_close_t *msg, uint8_t header);

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
typedef struct
{
    z_zint_t sn;
    z_zint_t count;
} _zn_sync_t;
void _zn_t_msg_clear_sync(_zn_sync_t *msg, uint8_t header);

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
typedef struct
{
    z_zint_t sn;
    z_zint_t mask;
} _zn_ack_nack_t;
void _zn_t_msg_clear_ack_nack(_zn_ack_nack_t *msg, uint8_t header);

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
// ~    peer_id    ~ if I==1 -- Peer ID of the KEEP_ALIVE sender.
// +---------------+
//
typedef struct
{
    z_bytes_t pid;
} _zn_keep_alive_t;
void _zn_t_msg_clear_keep_alive(_zn_keep_alive_t *msg, uint8_t header);

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
typedef struct
{
    z_zint_t hash;
} _zn_ping_pong_t;
void _zn_t_msg_clear_ping_pong(_zn_ping_pong_t *msg, uint8_t header);

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
// ~  FramePayload ~ -- if F==1 then the payload is a fragment of a single Zenoh Message, a list of complete Zenoh Messages otherwise.
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
typedef union
{
    _zn_payload_t fragment;
    _zn_zenoh_message_vec_t messages;
} _zn_frame_payload_t;
typedef struct
{
    z_zint_t sn;
    _zn_frame_payload_t payload;
} _zn_frame_t;
void _zn_t_msg_clear_frame(_zn_frame_t *msg, uint8_t header);

/*------------------ Transport Message ------------------*/
typedef union
{
    _zn_scout_t scout;
    _zn_hello_t hello;
    _zn_join_t join;
    _zn_init_t init;
    _zn_open_t open;
    _zn_close_t close;
    _zn_sync_t sync;
    _zn_ack_nack_t ack_nack;
    _zn_keep_alive_t keep_alive;
    _zn_ping_pong_t ping_pong;
    _zn_frame_t frame;
} _zn_transport_body_t;
typedef struct
{
    _zn_attachment_t *attachment;
    _zn_transport_body_t body;
    uint8_t header;
} _zn_transport_message_t;
void _zn_t_msg_clear(_zn_transport_message_t *msg);

/*------------------ Builders ------------------*/
_zn_transport_message_t _zn_t_msg_make_scout(uint8_t what, z_bytes_t zid);
_zn_transport_message_t _zn_t_msg_make_hello(uint8_t whatami, z_bytes_t zid, _zn_locator_array_t locators);
_zn_transport_message_t _zn_t_msg_make_join(uint8_t version, uint8_t whatami, z_zint_t lease, uint8_t sn_bs, z_bytes_t zid, _zn_conduit_sn_t next_sn);
_zn_transport_message_t _zn_t_msg_make_init_syn(uint8_t version, uint8_t whatami, uint8_t sn_bs, z_bytes_t zid);
_zn_transport_message_t _zn_t_msg_make_init_ack(uint8_t version, uint8_t whatami, uint8_t sn_bs, z_bytes_t zid, z_bytes_t cookie);
_zn_transport_message_t _zn_t_msg_make_open_syn(z_zint_t lease, z_zint_t initial_sn, z_bytes_t cookie);
_zn_transport_message_t _zn_t_msg_make_open_ack(z_zint_t lease, z_zint_t initial_sn);
_zn_transport_message_t _zn_t_msg_make_close(uint8_t reason, z_bytes_t pid, int link_only);
_zn_transport_message_t _zn_t_msg_make_sync(z_zint_t sn, int is_reliable, z_zint_t count);
_zn_transport_message_t _zn_t_msg_make_ack_nack(z_zint_t sn, z_zint_t mask);
_zn_transport_message_t _zn_t_msg_make_keep_alive(z_bytes_t pid);
_zn_transport_message_t _zn_t_msg_make_ping(z_zint_t hash);
_zn_transport_message_t _zn_t_msg_make_pong(z_zint_t hash);
_zn_transport_message_t _zn_t_msg_make_frame(z_zint_t sn, _zn_frame_payload_t payload, int is_reliable, int is_fragment, int is_final);
_zn_transport_message_t _zn_t_msg_make_frame_header(z_zint_t sn, int is_reliable, int is_fragment, int is_final);

/*------------------ Copy ------------------*/
// @TODO: implement the remaining copyers
void _zn_t_msg_copy(_zn_transport_message_t *clone, _zn_transport_message_t *msg);
void _zn_t_msg_copy_scout(_zn_scout_t *clone, _zn_scout_t *scout);
void _zn_t_msg_copy_hello(_zn_hello_t *clone, _zn_hello_t *hello);
void _zn_t_msg_copy_join(_zn_join_t *clone, _zn_join_t *join);
void _zn_t_msg_copy_init(_zn_init_t *clone, _zn_init_t *init);
void _zn_t_msg_copy_open(_zn_open_t *clone, _zn_open_t *open);
//void _zn_t_msg_copy_close(_zn_close_t *clone, _zn_close_t *close);
//void _zn_t_msg_copy_sync(_zn_sync_t *clone, _zn_sync_t *sync);
//void _zn_t_msg_copy_ack_nack(_zn_ack_nack_t *clone, _zn_ack_nack_t *ack);
//void _zn_t_msg_copy_keep_alive(_zn_keep_alive_t *clone, _zn_keep_alive_t *keep_alive);
//void _zn_t_msg_copy_ping(_zn_ping_pong_t *clone, _zn_ping_pong_t *ping);
//void _zn_t_msg_copy_pong(_zn_ping_pong_t *clone, _zn_ping_pong_t *pong);
//void _zn_t_msg_copy_frame(_zn_frame_t *clone, _zn_frame_t *frame);

#endif /* ZENOH_PICO_PROTOCOL_MSG_H */
