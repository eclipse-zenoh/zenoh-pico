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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_MSG_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_MSG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/array.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/keyexpr.h"

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
void _z_declaration_clear_resource(_z_res_decl_t *dcl);

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
void _z_declaration_clear_forget_resource(_z_forget_res_decl_t *dcl);

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
void _z_declaration_clear_publisher(_z_pub_decl_t *dcl);

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
void _z_declaration_clear_forget_publisher(_z_forget_pub_decl_t *dcl);

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
void _z_declaration_clear_subscriber(_z_sub_decl_t *dcl);

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
void _z_declaration_clear_forget_subscriber(_z_forget_sub_decl_t *dcl);

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
void _z_declaration_clear_queryable(_z_qle_decl_t *dcl);

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
void _z_declaration_clear_forget_queryable(_z_forget_qle_decl_t *dcl);

/*------------------ Timestamp Field ------------------*/
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// ~     Time      ~  Encoded as _z_zint_t
// +---------------+
// ~      ID       ~
// +---------------+
//
void _z_timestamp_clear(_z_timestamp_t *ts);

/*------------------ Builders ------------------*/
_z_network_message_t _z_n_msg_make_declare(_z_declaration_t declarations);
_z_network_message_t _z_n_msg_make_push(_z_keyexpr_t key, _z_push_body_t body, _Bool is_remote_mapping);
// _z_network_message_t _z_n_msg_make_request(_z_zint_t rid, _z_keyexpr_t key, _z_request_body_t body,
//                                            _Bool is_remote_mapping);
// _z_network_message_t _z_n_msg_make_response(_z_zint_t rid, _z_keyexpr_t key, _z_response_body_t body,
//                                             _Bool is_remote_mapping);
_z_network_message_t _z_n_msg_make_response_final(_z_zint_t rid);

/*------------------ Builders ------------------*/
// _z_zenoh_message_t _z_msg_make_data(_z_keyexpr_t key, _z_data_info_t info, _z_bytes_t payload, _Bool can_be_dropped);
_z_network_message_t _z_msg_make_unit(_Bool can_be_dropped);
_z_network_message_t _z_msg_make_query(_z_keyexpr_t key, _z_bytes_t parameters, _z_zint_t qid,
                                       z_consolidation_mode_t consolidation, _z_value_t value);
// _z_zenoh_message_t _z_msg_make_reply(_z_keyexpr_t key, _z_data_info_t info, _z_bytes_t payload, _Bool
// can_be_dropped);

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_MSG_H */
