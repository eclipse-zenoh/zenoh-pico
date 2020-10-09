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

//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X| RESOURCE|
// +---------------+
// ~     ResID     ~
// +---------------+
// ~    ResKey     ~ if  K==1 then only numerical id
// +---------------+
typedef struct
{
    z_zint_t id;
    zn_res_key_t key;
} _zn_res_decl_t;

typedef struct
{
    z_zint_t id;
    zn_res_key_t key;
    zn_data_handler_t data_handler;
    void *arg;
} _zn_sub_t;

typedef struct
{
    char *rname;
    z_zint_t rid;
    z_zint_t id;
    zn_query_handler_t query_handler;
    void *arg;
} _zn_qle_t;

/*------------------ Entity ------------------*/
z_zint_t _zn_get_entity_id(zn_session_t *z);

/*------------------ Resource ------------------*/
z_zint_t _zn_get_resource_id(zn_session_t *z, const zn_res_key_t *res_key);
_zn_res_decl_t *_zn_get_resource_by_id(z_list_t *resources, z_zint_t rid);
_zn_res_decl_t *_zn_get_resource_by_key(z_list_t *resources, const zn_res_key_t *res_key);
int _zn_register_resource(zn_session_t *z, int is_local, z_zint_t rid, const zn_res_key_t *res_key);
void _zn_unregister_resource(zn_session_t *z, int is_local, _zn_res_decl_t *res);

/*------------------ Subscription ------------------*/
z_list_t *_zn_get_subscriptions_from_remote_key(zn_session_t *z, const zn_res_key_t *res_key);
_zn_sub_t *_zn_get_subscription_by_id(z_list_t *subscriptions, z_zint_t id);
_zn_sub_t *_zn_get_subscription_by_key(z_list_t *subscriptions, const zn_res_key_t *res_key);
int _zn_register_subscription(zn_session_t *z, int is_local, z_zint_t id, const zn_res_key_t *res_key, zn_data_handler_t data_handler, void *arg);
void _zn_unregister_subscription(zn_sub_t *s);

// void _zn_register_queryable(zn_session_t *z, z_zint_t rid, z_zint_t id, zn_query_handler_t query_handler, void *arg);
// z_list_t *_zn_get_queryable_by_rid(zn_session_t *z, z_zint_t rid);
// z_list_t *_zn_get_queryable_by_rname(zn_session_t *z, const char *rname);
// void _zn_unregister_queryable(zn_qle_t *s);

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
