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

#ifndef ZENOH_C_NET_INTERNAL_H
#define ZENOH_C_NET_INTERNAL_H

#include "zenoh/net/types.h"
#include "zenoh/net/private/msg.h"

#define _ZN_IS_REMOTE 0
#define _ZN_IS_LOCAL 1

/*------------------ Message helper ------------------*/
_zn_session_message_t _zn_session_message_init(uint8_t header);
_zn_zenoh_message_t _zn_zenoh_message_init(uint8_t header);

/*------------------ SN helpers ------------------*/
int _zn_sn_precedes(z_zint_t sn_resolution_half, z_zint_t sn_left, z_zint_t sn_right);

/*------------------ Transmission and Reception helpers ------------------*/
int _zn_send_s_msg(zn_session_t *z, _zn_session_message_t *m);
int _zn_send_z_msg(zn_session_t *z, _zn_zenoh_message_t *m, zn_reliability_t reliability);

_zn_session_message_p_result_t _zn_recv_s_msg(zn_session_t *z);
void _zn_recv_s_msg_na(zn_session_t *z, _zn_session_message_p_result_t *r);

/*------------------ Entity ------------------*/
z_zint_t _zn_get_entity_id(zn_session_t *z);

/*------------------ Resource ------------------*/
z_zint_t _zn_get_resource_id(zn_session_t *z, const zn_reskey_t *res_key);
_zn_resource_t *_zn_get_resource_by_id(zn_session_t *z, int is_local, z_zint_t rid);
_zn_resource_t *_zn_get_resource_by_key(zn_session_t *z, int is_local, const zn_reskey_t *res_key);
z_str_t _zn_get_resource_name_from_key(zn_session_t *z, int is_local, const zn_reskey_t *res_key);

int _zn_register_resource(zn_session_t *z, int is_local, _zn_resource_t *res);
void _zn_unregister_resource(zn_session_t *z, int is_local, _zn_resource_t *res);

/*------------------ Subscription ------------------*/
_z_list_t *_zn_get_subscriptions_from_remote_key(zn_session_t *z, const zn_reskey_t *res_key);
_zn_subscriber_t *_zn_get_subscription_by_id(zn_session_t *z, int is_local, z_zint_t id);
_zn_subscriber_t *_zn_get_subscription_by_key(zn_session_t *z, int is_local, const zn_reskey_t *res_key);
int _zn_register_subscription(zn_session_t *z, int is_local, _zn_subscriber_t *s);
void _zn_unregister_subscription(zn_session_t *z, int is_local, _zn_subscriber_t *s);
void _zn_trigger_subscriptions(zn_session_t *z, const zn_reskey_t reskey, const z_bytes_t payload);

// void _zn_register_queryable(zn_session_t *z, z_zint_t rid, z_zint_t id, zn_query_handler_t query_handler, void *arg);
// _z_list_t *_zn_get_queryable_by_rid(zn_session_t *z, z_zint_t rid);
// _z_list_t *_zn_get_queryable_by_rname(zn_session_t *z, const char *rname);
// void _zn_unregister_queryable(zn_queryable_t *s);

// int _zn_matching_remote_sub(zn_session_t *z, z_zint_t rid);

// typedef struct
// {
//     z_zint_t qid;
//     zn_reply_handler_t reply_handler;
//     void *arg;
// } _zn_replywaiter_t;

// void _zn_register_query(zn_session_t *z, z_zint_t qid, zn_reply_handler_t reply_handler, void *arg);
// _zn_replywaiter_t *_zn_get_query(zn_session_t *z, z_zint_t qid);

#endif /* ZENOH_C_NET_INTERNAL_H */
