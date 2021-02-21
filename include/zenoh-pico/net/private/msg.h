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

#ifndef _ZENOH_NET_PICO_MSG_H
#define _ZENOH_NET_PICO_MSG_H

#include <stdint.h>
#include "zenoh-pico/net/private/types.h"

// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
#define _ZN_MSG_LEN_ENC_SIZE 2

/*=============================*/
/*         Message IDs         */
/*=============================*/
/* Session Messages */
#define _ZN_MID_SCOUT 0x01
#define _ZN_MID_HELLO 0x02
#define _ZN_MID_INIT 0x03
#define _ZN_MID_OPEN 0x04
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
/* Message decorators */
#define _ZN_MID_REPLY_CONTEXT 0x1e
#define _ZN_MID_ATTACHMENT 0x1f

/*=============================*/
/*        Message flags        */
/*=============================*/
/* Session message flags */
#define _ZN_FLAG_S_A 0x20 // 1 << 5 | Ack           if A==1 then the message is an acknowledgment
#define _ZN_FLAG_S_C 0x40 // 1 << 6 | Count         if C==1 then number of unacknowledged messages is present
#define _ZN_FLAG_S_E 0x80 // 1 << 7 | End           if E==1 then it is the last FRAME fragment
#define _ZN_FLAG_S_F 0x40 // 1 << 6 | Fragment      if F==1 then the FRAME is a fragment
#define _ZN_FLAG_S_I 0x20 // 1 << 5 | PeerID        if I==1 then the PeerID is present or requested
#define _ZN_FLAG_S_K 0x40 // 1 << 6 | CloseLink     if K==1 then close the transport link only
#define _ZN_FLAG_S_L 0x80 // 1 << 7 | Locators      if L==1 then Locators are present
#define _ZN_FLAG_S_M 0x20 // 1 << 5 | Mask          if M==1 then a Mask is present
#define _ZN_FLAG_S_P 0x20 // 1 << 5 | PingOrPong    if P==1 then the message is Ping, otherwise is Pong
#define _ZN_FLAG_S_R 0x20 // 1 << 5 | Reliable      if R==1 then it concerns the reliable channel, best-effort otherwise
#define _ZN_FLAG_S_S 0x40 // 1 << 6 | SN Resolution if S==1 then the SN Resolution is present
#define _ZN_FLAG_S_T 0x40 // 1 << 6 | TimeRes       if T==1 then the time resolution is in seconds
#define _ZN_FLAG_S_W 0x40 // 1 << 6 | WhatAmI       if W==1 then WhatAmI is indicated
#define _ZN_FLAG_S_X 0x00 // Unused flags are set to zero
/* Zenoh message flags */
#define _ZN_FLAG_Z_D 0x20 // 1 << 5 | Dropping      if D==1 then the message can be dropped
#define _ZN_FLAG_Z_F 0x20 // 1 << 5 | Final         if F==1 then this is the final message (e.g., ReplyContext, Pull)
#define _ZN_FLAG_Z_I 0x40 // 1 << 6 | DataInfo      if I==1 then DataInfo is present
#define _ZN_FLAG_Z_K 0x80 // 1 << 7 | ResourceKey   if K==1 then only numerical ID
#define _ZN_FLAG_Z_N 0x40 // 1 << 6 | MaxSamples    if N==1 then the MaxSamples is indicated
#define _ZN_FLAG_Z_P 0x80 // 1 << 7 | Period        if P==1 then a period is present
#define _ZN_FLAG_Z_R 0x20 // 1 << 5 | Reliable      if R==1 then it concerns the reliable channel, best-effort otherwise
#define _ZN_FLAG_Z_S 0x40 // 1 << 6 | SubMode       if S==1 then the declaration SubMode is indicated
#define _ZN_FLAG_Z_T 0x20 // 1 << 5 | QueryTarget   if T==1 then the query target is present
#define _ZN_FLAG_Z_X 0x00 // Unused flags are set to zero

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
/*     Attachment encodings    */
/*=============================*/
#define _ZN_ATT_ENC_PROPERTIES 0x00

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
#define _ZN_CLOSE_MAX_SESSIONS 0x03
#define _ZN_CLOSE_MAX_LINKS 0x04
#define _ZN_CLOSE_EXPIRED 0x05

/*=============================*/
/*       DataInfo flags        */
/*=============================*/
#define _ZN_DATA_INFO_SRC_ID 0x01 // 1 << 0
#define _ZN_DATA_INFO_SRC_SN 0x02 // 1 << 1
#define _ZN_DATA_INFO_RTR_ID 0x04 // 1 << 2
#define _ZN_DATA_INFO_RTR_SN 0x08 // 1 << 3
#define _ZN_DATA_INFO_TSTAMP 0x10 // 1 << 4
#define _ZN_DATA_INFO_KIND 0x20   // 1 << 5
#define _ZN_DATA_INFO_ENC 0x40    // 1 << 6

/*------------------ Payload field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// ~    Length     ~
// +---------------+
// ~    Buffer     ~
// +---------------+
//
typedef z_bytes_t _zn_payload_t;

/*------------------ Locators Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// ~  Num of Locs  ~
// +---------------+
// ~   [Locator]   ~
// +---------------+
//
// NOTE: Locators is a vector of strings and are encoded as such
//
typedef z_str_array_t _zn_locators_t;

/*=============================*/
/*     Message decorators      */
/*=============================*/
/*------------------ Attachment Decorator ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
//       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the length of a message must not exceed 65_535 bytes.
//
// The Attachment can decorate any message (i.e., SessionMessage and ZenohMessage) and it allows to
// append to the message any additional information. Since the information contained in the
// Attachement is relevant only to the layer that provided them (e.g., Session, Zenoh, User) it
// is the duty of that layer to serialize and de-serialize the attachment whenever deemed necessary.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// | ENC |  ATTCH  |
// +-+-+-+---------+
// ~    Buffer     ~
// +---------------+
//
// ENC values:
// - 0x00 => Zenoh Properties
//
typedef struct
{
    _zn_payload_t payload;
    uint8_t header;
} _zn_attachment_t;

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
// ~  source_kind  ~ //@TODO: make it if F==0
// +---------------+
// ~   replier_id  ~ if F==0
// +---------------+
//
// - if F==1 then the message is a REPLY_FINAL
//
typedef struct
{
    z_zint_t qid;
    z_zint_t source_kind;
    z_bytes_t replier_id;
    uint8_t header;
} _zn_reply_context_t;

/*=============================*/
/*      Session Messages       */
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
// - if I==1 then the sender is asking for hello replies that contain a peer_id.
//
typedef struct
{
    z_zint_t what;
} _zn_scout_t;

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
// ~    peer-id    ~ if I==1
// +---------------+
// ~    whatami    ~ if W==1 -- Otherwise it is from a Router
// +---------------+
// ~    Locators   ~ if L==1 -- Otherwise src-address is the locator
// +---------------+
//
typedef struct
{
    z_zint_t whatami;
    z_bytes_t pid;
    _zn_locators_t locators;
} _zn_hello_t;

/*------------------ Init Message ------------------*/
// NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total lenght
//       in bytes of the message, resulting in the maximum lenght of a message being 65_535 bytes.
//       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
//       the boundary of the serialized messages. The length is encoded as little-endian.
//       In any case, the lenght of a message must not exceed 65_535 bytes.
//
// The INIT message is sent on a specific Locator to initiate a session with the peer associated
// with that Locator. The initiator MUST send an INIT message with the A flag set to 0.  If the
// corresponding peer deems appropriate to initialize a session with the initiator, the corresponding
// peer MUST reply with an INIT message with the A flag set to 1.
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|S|A|   INIT  |
// +-+-+-+-+-------+
// | v_maj | v_min | if A==0 -- Protocol Version VMaj.VMin
// +-------+-------+
// ~    whatami    ~ -- Client, Router, Peer or a combination of them
// +---------------+
// ~    peer_id    ~ -- PID of the sender of the INIT message
// +---------------+
// ~ sn_resolution ~ if S==1(*) -- Otherwise 2^28 is assumed(**)
// +---------------+
// ~     cookie    ~ if A==1
// +---------------+
//
// (*) if A==0 and S==0 then 2^28 is assumed.
//     if A==1 and S==0 then the agreed resolution is the one communicated by the initiator.
// ```
typedef struct
{
    z_zint_t whatami;
    z_zint_t sn_resolution;
    z_bytes_t pid;
    z_bytes_t cookie;
    uint8_t version;
} _zn_init_t;

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
// ```
typedef struct
{
    z_zint_t lease;
    z_zint_t initial_sn;
    z_bytes_t cookie;
} _zn_open_t;

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
typedef struct
{
    z_zint_t sn;
    union
    {
        _zn_payload_t fragment;
        _z_vec_t messages;
    } payload;
} _zn_frame_t;

/*------------------ Session Message ------------------*/
typedef struct
{
    _zn_attachment_t *attachment;
    union
    {
        _zn_scout_t scout;
        _zn_hello_t hello;
        _zn_init_t init;
        _zn_open_t open;
        _zn_close_t close;
        _zn_sync_t sync;
        _zn_ack_nack_t ack_nack;
        _zn_keep_alive_t keep_alive;
        _zn_ping_pong_t ping_pong;
        _zn_frame_t frame;
    } body;
    uint8_t header;
} _zn_session_message_t;

/*=============================*/
/*       Zenoh Messages        */
/*=============================*/
/*------------------ ResKey Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// ~      RID      ~
// +---------------+
// ~    Suffix     ~ if K==0. Otherwise: only numerical RID
// +---------------+
//
// typdef struct { ... } zn_reskey_t; is defined in zenoh/net/types.h

/*------------------ Resource Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|   RES   |
// +---------------+
// ~      RID      ~
// +---------------+
// ~    ResKey     ~
// +---------------+
//
typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
} _zn_res_decl_t;

/*------------------ Publisher Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|   PUB   |
// +---------------+
// ~    ResKey     ~
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
} _zn_pub_decl_t;

/*------------------ SubInfo Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// |P|X|X|  SM_ID  |
// +---------------+
// ~    Period     ~ if P==1
// +---------------+
//
// typdef struct { ... } zn_subinfo_t; is defined in net/types.h

/*------------------ Subscriber Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|S|R|   SUB   |
// +---------------+
// ~    ResKey     ~
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

/*------------------ Queryable Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|  QABLE  |
// +---------------+
// ~     ResKey    ~
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
} _zn_qle_decl_t;

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

/*------------------ Forget Publisher Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|  F_PUB  |
// +---------------+
// ~    ResKey     ~
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
} _zn_forget_pub_decl_t;

/*------------------ Forget Subscriber Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X|  F_SUB  |
// +---------------+
// ~    ResKey     ~
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
} _zn_forget_sub_decl_t;

/*------------------ Forget Queryable Declaration ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X| F_QABLE |
// +---------------+
// ~    ResKey     ~
// +---------------+
//
typedef struct
{
    zn_reskey_t key;
} _zn_forget_qle_decl_t;

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
_ARRAY_DECLARE(_zn_declaration_t, declaration, _zn_)

typedef struct
{
    _zn_declaration_array_t declarations;
} _zn_declare_t;

/*------------------ Timestamp Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// ~     Time      ~  Encoded as z_zint_t
// +---------------+
// ~      ID       ~
// +---------------+
//
// typdef struct { ... } z_timestamp_t; is defined in zenoh/types.h

/*------------------ Data Info Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// ~     flags     ~ -- encoded as z_zint_t
// +---------------+
// ~   source_id   ~ if _ZN_DATA_INFO_SRC_ID==1
// +---------------+
// ~   source_sn   ~ if _ZN_DATA_INFO_SRC_SN==1
// +---------------+
// ~first_router_id~ if _ZN_DATA_INFO_RTR_ID==1
// +---------------+
// ~first_router_sn~ if _ZN_DATA_INFO_RTR_SN==1
// +---------------+
// ~   timestamp   ~ if _ZN_DATA_INFO_TSTAMP==1
// +---------------+
// ~      kind     ~ if _ZN_DATA_INFO_KIND==1
// +---------------+
// ~   encoding    ~ if _ZN_DATA_INFO_ENC==1
// +---------------+
//
typedef struct
{
    z_zint_t flags;
    z_bytes_t source_id;
    z_zint_t source_sn;
    z_bytes_t first_router_id;
    z_zint_t first_router_sn;
    z_timestamp_t tstamp;
    z_zint_t kind;
    z_zint_t encoding;
} _zn_data_info_t;

/*------------------ Data Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|I|D|  DATA   |
// +-+-+-+---------+
// ~    ResKey     ~
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

/*------------------ Unit Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |X|X|D|  UNIT   |
// +-+-+-+---------+
//
// - if D==1 then the message can be dropped for congestion control reasons.
//
// NOTE: Unit messages have no body

/*------------------ Pull Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|N|F|  PULL   |
// +-+-+-+---------+
// ~    ResKey     ~
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

/*------------------ Query Message ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|T|  QUERY  |
// +-+-+-+---------+
// ~    ResKey     ~
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

/*------------------ Zenoh Message ------------------*/
typedef struct
{
    _zn_attachment_t *attachment;
    _zn_reply_context_t *reply_context;
    union
    {
        _zn_declare_t declare;
        _zn_data_t data;
        _zn_query_t query;
        _zn_pull_t pull;
    } body;
    uint8_t header;
} _zn_zenoh_message_t;

#endif /* _ZENOH_NET_PICO_MSG_H */
